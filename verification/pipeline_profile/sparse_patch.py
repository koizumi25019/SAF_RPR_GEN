"""Apply only to an isolated source copy; retain all CNF clause/variable order."""
import re

def patch(tree):
 def edit(name,old,new):
  p=tree/'src'/name;s=p.read_text();assert s.count(old)==1,(name,old,s.count(old));p.write_text(s.replace(old,new))
 f='fdp/cnf/faulty_circuit.c'
 edit(f,'RESET_FLAG;\n\tRESET_VARSFC;', 'SparseResetTFO();')
 edit(f,'numtrannet++;','SparseRecord(netptr->n);\n\t\t\tnumtrannet++;')
 edit(f,'\t/** create the faulty-circuit constraint */','\tSparseFinish();\n\t/** create the faulty-circuit constraint */')
 edit(f,'for (int j = 0; j < n_net; j++)\n\t{','for (int sparse_k=0; sparse_k<SparseCount(); sparse_k++)\n\t{\n        int j=SparseNet(sparse_k);')
 edit(f,'for (int i = 0; i < n_net; i++)\n\t{','for (int sparse_k=0; sparse_k<SparseCount(); sparse_k++)\n\t{\n        int i=SparseNet(sparse_k);')
 edit('fdp/cnf/detection_circuit.c','for (int i = 0; i < n_net; i++)\n\t{','for (int sparse_k=0; sparse_k<SparseCount(); sparse_k++)\n\t{\n        int i=SparseNet(sparse_k);')
 f='fdp/essential_assignment.c'
 edit(f,'''    for (int i = 0; i < n_net; i++) {
        nl[i].logic_value = EA_UNKNOWN;
        nl[i].ea_flag     = EA_DOWN;
        nl[i].unique_flag = UNIQUE_DOWN;
    }''','    SparseEAReset();')
 p=tree/'src'/f;s=p.read_text()
 s,n=re.subn(r'^(\s*)([\w>\-\[\].]+)->(ea_flag|unique_flag)\s*=\s*(EA_UP|UNIQUE_UP|UNIQUE_MIDDLE);',
             lambda m:f'{m[1]}SparseEAMark({m[2]});{m[0]}',s,flags=re.M)
 assert n==6,n
 s=s.replace('que2.que    = (NLIST**)allocMemory(que2.maxnum, sizeof(NLIST*));','if (!que2.que) que2.que = (NLIST**)allocMemory(que2.maxnum, sizeof(NLIST*));')
 s=s.replace('    free(que2.que);\n    que2.que = NULL;','    /* persistent scratch queue */')
 p.write_text(s)
