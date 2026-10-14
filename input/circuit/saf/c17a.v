module c17(a, b, c, d, e, k, l);
	input  a, b, c, d, e;
	output k, l;
	wire   a, b, c, d, e, f, g, h, i, j, k, l;

	INV  G0 ( .A(c), .Z(f) );
	AND2 G1 ( .A(f), .B(a), .Z(g) );
	OR2  G2 ( .A(c), .B(d), .Z(h) );
	AND2 G3 ( .A(b), .B(h), .Z(i) );
	AND2 G4 ( .A(e), .B(h), .Z(j) );
	OR2  G5 ( .A(g), .B(i), .Z(k) );
	OR2  G6 ( .A(i), .B(j), .Z(l) );

endmodule
