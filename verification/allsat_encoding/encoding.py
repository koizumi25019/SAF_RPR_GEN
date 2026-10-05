"""NNF + Plaisted--Greenbaum encoding, with both detection polarities.

The selector has the original function as its existential projection. Negating
the root of a one-directional PG encoding alone would NOT encode non-detection.
Shared positive/negative DAG nodes avoid exponential expansion of XOR chains.
"""


class Encoded:
    def __init__(self, expression, mode):
        self.expression = expression
        self.mode = mode

    def __getattr__(self, name):
        return getattr(self.expression, name)

    def cnf(self, output, dual=True):
        if self.mode == 'tseitin':
            return self.expression.cnf(output)
        assert self.mode in ('nnf_pg', 'nnf_pg_split')
        clauses = []
        nv = self.n
        memo = {}
        gates = {}

        def fresh():
            nonlocal nv
            nv += 1
            return nv

        one = fresh()
        clauses.append([one])

        def gate(kind, children):
            children = tuple(sorted(set(children)))
            if len(children) == 1:
                return children[0]
            if not children:
                return one if kind == 'and' else -one
            key = kind, children
            if key not in gates:
                z = fresh()
                if kind == 'and':
                    clauses.extend([[-z, c] for c in children])
                else:
                    clauses.append([-z, *children])
                gates[key] = z
            return gates[key]

        def parity(args, neg):
            key = ('parity', args, neg)
            if key not in memo:
                if len(args) == 1:
                    result = visit(args[0] ^ neg)
                else:
                    a, rest = args[0], args[1:]
                    # XOR (neg=0): (a & !rest) | (!a & rest).
                    result = gate('or', [
                        gate('and', [visit(a), parity(rest, 1 ^ neg)]),
                        gate('and', [visit(a ^ 1), parity(rest, neg)])])
                memo[key] = result
            return memo[key]

        def visit(lit):
            if lit in memo:
                return memo[lit]
            node, sign = divmod(lit, 2)
            if node == 0:
                return one if sign else -one
            if node <= self.n:
                return -node if sign else node
            kind, args = self.nodes[node]
            if kind == 'a':
                result = gate('or' if sign else 'and', [visit(a ^ sign) for a in args])
            else:
                assert kind == 'x'
                result = parity(tuple(args), sign)
            memo[lit] = result
            return result

        positive = visit(output)
        if not dual:
            return clauses, nv, positive
        negative = visit(output ^ 1)
        selector = fresh()
        if self.mode == 'nnf_pg_split':
            definitions = {z:children for (_,children),z in gates.items()}
            def closure(root):
                seen = set()
                stack = [abs(root)]
                while stack:
                    node = stack.pop()
                    if node in seen:
                        continue
                    seen.add(node)
                    stack.extend(abs(c) for c in definitions.get(node, ()))
                return [c for c in clauses if abs(c[0]) in seen or c == [one]]
            self.off_clauses = closure(negative)+[[selector,negative]]
            return closure(positive)+[[-selector,positive]], nv, selector
        clauses.extend([[-selector, positive], [selector, negative]])
        return clauses, nv, selector
