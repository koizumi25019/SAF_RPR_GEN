
module s27_C ( G7, G6, G5, G3, G2, G17, G13, G11, G10, G1, G0 );
  input G7, G6, G5, G3, G2, G1, G0;
  output G17, G13, G11, G10;
  wire   G12, G14, G15, G16, G8, G9;

  NAND2 gate12 ( .A(G16), .B(G15), .Z(G9) );
  OR2 gate11 ( .A(G3), .B(G8), .Z(G16) );
  OR2 gate10 ( .A(G12), .B(G8), .Z(G15) );
  NOR2 gate9 ( .A(G1), .B(G7), .Z(G12) );
  AND2 gate8 ( .A(G14), .B(G6), .Z(G8) );
  INV gate7 ( .A(G0), .Z(G14) );
  NOR2 gate5 ( .A(G2), .B(G12), .Z(G13) );
  NOR2 gate3 ( .A(G5), .B(G9), .Z(G11) );
  NOR2 gate1 ( .A(G14), .B(G11), .Z(G10) );
  INV gate0 ( .A(G11), .Z(G17) );
endmodule
