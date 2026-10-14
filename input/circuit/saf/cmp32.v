module Cmp32 ( g, left, right );
  input [31:0] left;
  input [31:0] right;
  output g;
  wire   \less0/n1 , \less1/n3 , \less1/n2 , \less1/n1 , \less2/n6 ,
         \less2/n5 , \less2/n4 , \less31/n6 , \less31/n5 , \less31/n4 ,
         \less30/n6 , \less30/n5 , \less30/n4 , \less29/n6 , \less29/n5 ,
         \less29/n4 , \less28/n6 , \less28/n5 , \less28/n4 , \less27/n6 ,
         \less27/n5 , \less27/n4 , \less26/n6 , \less26/n5 , \less26/n4 ,
         \less25/n6 , \less25/n5 , \less25/n4 , \less24/n6 , \less24/n5 ,
         \less24/n4 , \less23/n6 , \less23/n5 , \less23/n4 , \less22/n6 ,
         \less22/n5 , \less22/n4 , \less21/n6 , \less21/n5 , \less21/n4 ,
         \less20/n6 , \less20/n5 , \less20/n4 , \less19/n6 , \less19/n5 ,
         \less19/n4 , \less18/n6 , \less18/n5 , \less18/n4 , \less17/n6 ,
         \less17/n5 , \less17/n4 , \less16/n6 , \less16/n5 , \less16/n4 ,
         \less15/n6 , \less15/n5 , \less15/n4 , \less14/n6 , \less14/n5 ,
         \less14/n4 , \less13/n6 , \less13/n5 , \less13/n4 , \less12/n6 ,
         \less12/n5 , \less12/n4 , \less11/n6 , \less11/n5 , \less11/n4 ,
         \less10/n6 , \less10/n5 , \less10/n4 , \less9/n6 , \less9/n5 ,
         \less9/n4 , \less8/n6 , \less8/n5 , \less8/n4 , \less7/n6 ,
         \less7/n5 , \less7/n4 , \less6/n6 , \less6/n5 , \less6/n4 ,
         \less5/n6 , \less5/n5 , \less5/n4 , \less4/n6 , \less4/n5 ,
         \less4/n4 , \less3/n6 , \less3/n5 , \less3/n4 ;
  wire   [30:0] less_out;

  INV \less0/U2  ( .A(right[0]), .Z(\less0/n1 ) );
  NOR2 \less0/U1  ( .A(left[0]), .B(\less0/n1 ), .Z(less_out[0]) );
  NOR2 \less1/U6  ( .A(less_out[0]), .B(right[1]), .Z(\less1/n3 ) );
  OR2 \less1/U5  ( .A(left[1]), .B(\less1/n3 ), .Z(\less1/n1 ) );
  NAND2 \less1/U4  ( .A(less_out[0]), .B(right[1]), .Z(\less1/n2 ) );
  NAND2 \less1/U3  ( .A(\less1/n1 ), .B(\less1/n2 ), .Z(less_out[1]) );
  NOR2 \less2/U6  ( .A(less_out[1]), .B(right[2]), .Z(\less2/n4 ) );
  OR2 \less2/U5  ( .A(left[2]), .B(\less2/n4 ), .Z(\less2/n6 ) );
  NAND2 \less2/U4  ( .A(less_out[1]), .B(right[2]), .Z(\less2/n5 ) );
  NAND2 \less2/U3  ( .A(\less2/n6 ), .B(\less2/n5 ), .Z(less_out[2]) );
  NOR2 \less31/U6  ( .A(less_out[30]), .B(right[31]), .Z(\less31/n4 ) );
  OR2 \less31/U5  ( .A(left[31]), .B(\less31/n4 ), .Z(\less31/n6 ) );
  NAND2 \less31/U4  ( .A(less_out[30]), .B(right[31]), .Z(\less31/n5 ) );
  NAND2 \less31/U3  ( .A(\less31/n6 ), .B(\less31/n5 ), .Z(g) );
  NOR2 \less30/U6  ( .A(less_out[29]), .B(right[30]), .Z(\less30/n4 ) );
  OR2 \less30/U5  ( .A(left[30]), .B(\less30/n4 ), .Z(\less30/n6 ) );
  NAND2 \less30/U4  ( .A(less_out[29]), .B(right[30]), .Z(\less30/n5 ) );
  NAND2 \less30/U3  ( .A(\less30/n6 ), .B(\less30/n5 ), .Z(less_out[30]) );
  NOR2 \less29/U6  ( .A(less_out[28]), .B(right[29]), .Z(\less29/n4 ) );
  OR2 \less29/U5  ( .A(left[29]), .B(\less29/n4 ), .Z(\less29/n6 ) );
  NAND2 \less29/U4  ( .A(less_out[28]), .B(right[29]), .Z(\less29/n5 ) );
  NAND2 \less29/U3  ( .A(\less29/n6 ), .B(\less29/n5 ), .Z(less_out[29]) );
  NOR2 \less28/U6  ( .A(less_out[27]), .B(right[28]), .Z(\less28/n4 ) );
  OR2 \less28/U5  ( .A(left[28]), .B(\less28/n4 ), .Z(\less28/n6 ) );
  NAND2 \less28/U4  ( .A(less_out[27]), .B(right[28]), .Z(\less28/n5 ) );
  NAND2 \less28/U3  ( .A(\less28/n6 ), .B(\less28/n5 ), .Z(less_out[28]) );
  NOR2 \less27/U6  ( .A(less_out[26]), .B(right[27]), .Z(\less27/n4 ) );
  OR2 \less27/U5  ( .A(left[27]), .B(\less27/n4 ), .Z(\less27/n6 ) );
  NAND2 \less27/U4  ( .A(less_out[26]), .B(right[27]), .Z(\less27/n5 ) );
  NAND2 \less27/U3  ( .A(\less27/n6 ), .B(\less27/n5 ), .Z(less_out[27]) );
  NOR2 \less26/U6  ( .A(less_out[25]), .B(right[26]), .Z(\less26/n4 ) );
  OR2 \less26/U5  ( .A(left[26]), .B(\less26/n4 ), .Z(\less26/n6 ) );
  NAND2 \less26/U4  ( .A(less_out[25]), .B(right[26]), .Z(\less26/n5 ) );
  NAND2 \less26/U3  ( .A(\less26/n6 ), .B(\less26/n5 ), .Z(less_out[26]) );
  NOR2 \less25/U6  ( .A(less_out[24]), .B(right[25]), .Z(\less25/n4 ) );
  OR2 \less25/U5  ( .A(left[25]), .B(\less25/n4 ), .Z(\less25/n6 ) );
  NAND2 \less25/U4  ( .A(less_out[24]), .B(right[25]), .Z(\less25/n5 ) );
  NAND2 \less25/U3  ( .A(\less25/n6 ), .B(\less25/n5 ), .Z(less_out[25]) );
  NOR2 \less24/U6  ( .A(less_out[23]), .B(right[24]), .Z(\less24/n4 ) );
  OR2 \less24/U5  ( .A(left[24]), .B(\less24/n4 ), .Z(\less24/n6 ) );
  NAND2 \less24/U4  ( .A(less_out[23]), .B(right[24]), .Z(\less24/n5 ) );
  NAND2 \less24/U3  ( .A(\less24/n6 ), .B(\less24/n5 ), .Z(less_out[24]) );
  NOR2 \less23/U6  ( .A(less_out[22]), .B(right[23]), .Z(\less23/n4 ) );
  OR2 \less23/U5  ( .A(left[23]), .B(\less23/n4 ), .Z(\less23/n6 ) );
  NAND2 \less23/U4  ( .A(less_out[22]), .B(right[23]), .Z(\less23/n5 ) );
  NAND2 \less23/U3  ( .A(\less23/n6 ), .B(\less23/n5 ), .Z(less_out[23]) );
  NOR2 \less22/U6  ( .A(less_out[21]), .B(right[22]), .Z(\less22/n4 ) );
  OR2 \less22/U5  ( .A(left[22]), .B(\less22/n4 ), .Z(\less22/n6 ) );
  NAND2 \less22/U4  ( .A(less_out[21]), .B(right[22]), .Z(\less22/n5 ) );
  NAND2 \less22/U3  ( .A(\less22/n6 ), .B(\less22/n5 ), .Z(less_out[22]) );
  NOR2 \less21/U6  ( .A(less_out[20]), .B(right[21]), .Z(\less21/n4 ) );
  OR2 \less21/U5  ( .A(left[21]), .B(\less21/n4 ), .Z(\less21/n6 ) );
  NAND2 \less21/U4  ( .A(less_out[20]), .B(right[21]), .Z(\less21/n5 ) );
  NAND2 \less21/U3  ( .A(\less21/n6 ), .B(\less21/n5 ), .Z(less_out[21]) );
  NOR2 \less20/U6  ( .A(less_out[19]), .B(right[20]), .Z(\less20/n4 ) );
  OR2 \less20/U5  ( .A(left[20]), .B(\less20/n4 ), .Z(\less20/n6 ) );
  NAND2 \less20/U4  ( .A(less_out[19]), .B(right[20]), .Z(\less20/n5 ) );
  NAND2 \less20/U3  ( .A(\less20/n6 ), .B(\less20/n5 ), .Z(less_out[20]) );
  NOR2 \less19/U6  ( .A(less_out[18]), .B(right[19]), .Z(\less19/n4 ) );
  OR2 \less19/U5  ( .A(left[19]), .B(\less19/n4 ), .Z(\less19/n6 ) );
  NAND2 \less19/U4  ( .A(less_out[18]), .B(right[19]), .Z(\less19/n5 ) );
  NAND2 \less19/U3  ( .A(\less19/n6 ), .B(\less19/n5 ), .Z(less_out[19]) );
  NOR2 \less18/U6  ( .A(less_out[17]), .B(right[18]), .Z(\less18/n4 ) );
  OR2 \less18/U5  ( .A(left[18]), .B(\less18/n4 ), .Z(\less18/n6 ) );
  NAND2 \less18/U4  ( .A(less_out[17]), .B(right[18]), .Z(\less18/n5 ) );
  NAND2 \less18/U3  ( .A(\less18/n6 ), .B(\less18/n5 ), .Z(less_out[18]) );
  NOR2 \less17/U6  ( .A(less_out[16]), .B(right[17]), .Z(\less17/n4 ) );
  OR2 \less17/U5  ( .A(left[17]), .B(\less17/n4 ), .Z(\less17/n6 ) );
  NAND2 \less17/U4  ( .A(less_out[16]), .B(right[17]), .Z(\less17/n5 ) );
  NAND2 \less17/U3  ( .A(\less17/n6 ), .B(\less17/n5 ), .Z(less_out[17]) );
  NOR2 \less16/U6  ( .A(less_out[15]), .B(right[16]), .Z(\less16/n4 ) );
  OR2 \less16/U5  ( .A(left[16]), .B(\less16/n4 ), .Z(\less16/n6 ) );
  NAND2 \less16/U4  ( .A(less_out[15]), .B(right[16]), .Z(\less16/n5 ) );
  NAND2 \less16/U3  ( .A(\less16/n6 ), .B(\less16/n5 ), .Z(less_out[16]) );
  NOR2 \less15/U6  ( .A(less_out[14]), .B(right[15]), .Z(\less15/n4 ) );
  OR2 \less15/U5  ( .A(left[15]), .B(\less15/n4 ), .Z(\less15/n6 ) );
  NAND2 \less15/U4  ( .A(less_out[14]), .B(right[15]), .Z(\less15/n5 ) );
  NAND2 \less15/U3  ( .A(\less15/n6 ), .B(\less15/n5 ), .Z(less_out[15]) );
  NOR2 \less14/U6  ( .A(less_out[13]), .B(right[14]), .Z(\less14/n4 ) );
  OR2 \less14/U5  ( .A(left[14]), .B(\less14/n4 ), .Z(\less14/n6 ) );
  NAND2 \less14/U4  ( .A(less_out[13]), .B(right[14]), .Z(\less14/n5 ) );
  NAND2 \less14/U3  ( .A(\less14/n6 ), .B(\less14/n5 ), .Z(less_out[14]) );
  NOR2 \less13/U6  ( .A(less_out[12]), .B(right[13]), .Z(\less13/n4 ) );
  OR2 \less13/U5  ( .A(left[13]), .B(\less13/n4 ), .Z(\less13/n6 ) );
  NAND2 \less13/U4  ( .A(less_out[12]), .B(right[13]), .Z(\less13/n5 ) );
  NAND2 \less13/U3  ( .A(\less13/n6 ), .B(\less13/n5 ), .Z(less_out[13]) );
  NOR2 \less12/U6  ( .A(less_out[11]), .B(right[12]), .Z(\less12/n4 ) );
  OR2 \less12/U5  ( .A(left[12]), .B(\less12/n4 ), .Z(\less12/n6 ) );
  NAND2 \less12/U4  ( .A(less_out[11]), .B(right[12]), .Z(\less12/n5 ) );
  NAND2 \less12/U3  ( .A(\less12/n6 ), .B(\less12/n5 ), .Z(less_out[12]) );
  NOR2 \less11/U6  ( .A(less_out[10]), .B(right[11]), .Z(\less11/n4 ) );
  OR2 \less11/U5  ( .A(left[11]), .B(\less11/n4 ), .Z(\less11/n6 ) );
  NAND2 \less11/U4  ( .A(less_out[10]), .B(right[11]), .Z(\less11/n5 ) );
  NAND2 \less11/U3  ( .A(\less11/n6 ), .B(\less11/n5 ), .Z(less_out[11]) );
  NOR2 \less10/U6  ( .A(less_out[9]), .B(right[10]), .Z(\less10/n4 ) );
  OR2 \less10/U5  ( .A(left[10]), .B(\less10/n4 ), .Z(\less10/n6 ) );
  NAND2 \less10/U4  ( .A(less_out[9]), .B(right[10]), .Z(\less10/n5 ) );
  NAND2 \less10/U3  ( .A(\less10/n6 ), .B(\less10/n5 ), .Z(less_out[10]) );
  NOR2 \less9/U6  ( .A(less_out[8]), .B(right[9]), .Z(\less9/n4 ) );
  OR2 \less9/U5  ( .A(left[9]), .B(\less9/n4 ), .Z(\less9/n6 ) );
  NAND2 \less9/U4  ( .A(less_out[8]), .B(right[9]), .Z(\less9/n5 ) );
  NAND2 \less9/U3  ( .A(\less9/n6 ), .B(\less9/n5 ), .Z(less_out[9]) );
  NOR2 \less8/U6  ( .A(less_out[7]), .B(right[8]), .Z(\less8/n4 ) );
  OR2 \less8/U5  ( .A(left[8]), .B(\less8/n4 ), .Z(\less8/n6 ) );
  NAND2 \less8/U4  ( .A(less_out[7]), .B(right[8]), .Z(\less8/n5 ) );
  NAND2 \less8/U3  ( .A(\less8/n6 ), .B(\less8/n5 ), .Z(less_out[8]) );
  NOR2 \less7/U6  ( .A(less_out[6]), .B(right[7]), .Z(\less7/n4 ) );
  OR2 \less7/U5  ( .A(left[7]), .B(\less7/n4 ), .Z(\less7/n6 ) );
  NAND2 \less7/U4  ( .A(less_out[6]), .B(right[7]), .Z(\less7/n5 ) );
  NAND2 \less7/U3  ( .A(\less7/n6 ), .B(\less7/n5 ), .Z(less_out[7]) );
  NOR2 \less6/U6  ( .A(less_out[5]), .B(right[6]), .Z(\less6/n4 ) );
  OR2 \less6/U5  ( .A(left[6]), .B(\less6/n4 ), .Z(\less6/n6 ) );
  NAND2 \less6/U4  ( .A(less_out[5]), .B(right[6]), .Z(\less6/n5 ) );
  NAND2 \less6/U3  ( .A(\less6/n6 ), .B(\less6/n5 ), .Z(less_out[6]) );
  NOR2 \less5/U6  ( .A(less_out[4]), .B(right[5]), .Z(\less5/n4 ) );
  OR2 \less5/U5  ( .A(left[5]), .B(\less5/n4 ), .Z(\less5/n6 ) );
  NAND2 \less5/U4  ( .A(less_out[4]), .B(right[5]), .Z(\less5/n5 ) );
  NAND2 \less5/U3  ( .A(\less5/n6 ), .B(\less5/n5 ), .Z(less_out[5]) );
  NOR2 \less4/U6  ( .A(less_out[3]), .B(right[4]), .Z(\less4/n4 ) );
  OR2 \less4/U5  ( .A(left[4]), .B(\less4/n4 ), .Z(\less4/n6 ) );
  NAND2 \less4/U4  ( .A(less_out[3]), .B(right[4]), .Z(\less4/n5 ) );
  NAND2 \less4/U3  ( .A(\less4/n6 ), .B(\less4/n5 ), .Z(less_out[4]) );
  NOR2 \less3/U6  ( .A(less_out[2]), .B(right[3]), .Z(\less3/n4 ) );
  OR2 \less3/U5  ( .A(left[3]), .B(\less3/n4 ), .Z(\less3/n6 ) );
  NAND2 \less3/U4  ( .A(less_out[2]), .B(right[3]), .Z(\less3/n5 ) );
  NAND2 \less3/U3  ( .A(\less3/n6 ), .B(\less3/n5 ), .Z(less_out[3]) );
endmodule

