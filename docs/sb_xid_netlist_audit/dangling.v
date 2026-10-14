module dangling_example(a,b,y);
input a,b;
output y;
wire d;
AND2 u_unused(.A(a),.B(b),.Z(d));
OR2 u_out(.A(a),.B(b),.Z(y));
endmodule
