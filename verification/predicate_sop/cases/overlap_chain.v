module overlap_chain(x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13, z);
input x0, x1, x2, x3, x4, x5, x6, x7, x8, x9, x10, x11, x12, x13;
output z;
wire a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12;
OR2 g0 (.A(x0), .B(x1), .Z(a0));
OR2 g1 (.A(x1), .B(x2), .Z(a1));
OR2 g2 (.A(x2), .B(x3), .Z(a2));
OR2 g3 (.A(x3), .B(x4), .Z(a3));
OR2 g4 (.A(x4), .B(x5), .Z(a4));
OR2 g5 (.A(x5), .B(x6), .Z(a5));
OR2 g6 (.A(x6), .B(x7), .Z(a6));
OR2 g7 (.A(x7), .B(x8), .Z(a7));
OR2 g8 (.A(x8), .B(x9), .Z(a8));
OR2 g9 (.A(x9), .B(x10), .Z(a9));
OR2 g10 (.A(x10), .B(x11), .Z(a10));
OR2 g11 (.A(x11), .B(x12), .Z(a11));
OR2 g12 (.A(x12), .B(x13), .Z(a12));
AND13 last (.A(a0), .B(a1), .C(a2), .D(a3), .E(a4), .F(a5), .G(a6), .H(a7), .I(a8), .J(a9), .K(a10), .L(a11), .M(a12), .Z(z));
endmodule
