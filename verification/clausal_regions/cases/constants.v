module constants;
input x;
output zero, one;
INV i0 (.A(x), .Z(nx));
AND2 a0 (.A(x), .B(nx), .Z(zero));
OR2 o0 (.A(x), .B(nx), .Z(one));
endmodule
