
module s208_C ( Z, Y_8, Y_7, Y_6, Y_5, Y_4, Y_3, Y_2, Y_1, X, W, II6, II5, II4, 
        II3, II158, II157, II156, II155, Clear, C_8, C_7, C_6, C_5, C_4, C_3, 
        C_2, C_1, C_0 );
  input Y_8, Y_7, Y_6, Y_5, Y_4, Y_3, Y_2, Y_1, X, Clear, C_8, C_7, C_6, C_5,
         C_4, C_3, C_2, C_1, C_0;
  output Z, W, II6, II5, II4, II3, II158, II157, II156, II155;
  wire   II100, II104, II109, II113, II127_1, II127_2, II131_1, II131_2,
         II135_1, II135_2, II192, II193, II194, II195, II196, II198, II199,
         II1_1, II202, II244, II248, II252, II256, II261, II265, II279_1,
         II279_2, II283_1, II283_2, II287_1, II287_2, II307_1, II309, II310,
         II311, II314, II316, II317, II318, II341, II347, II350, II368, II369,
         II370, II371, II372, II374, II378, II40, II406, II409, II41, II42,
         II43, II44, II446, II46, II47, II484, II487, II488, II489, II490,
         II494, II495, II497_1, II50, II500_1, II500_2, II504_1, II504_2,
         II508_1, II508_2, II512_1, II512_2, II92, II96, P_1, P_2, P_3, P_4,
         P_5, P_6, P_7, P_8;

  NAND2 gate103 ( .A(II484), .B(II494), .Z(II495) );
  NOR3 gate102 ( .A(II512_1), .B(II512_2), .C(II489), .Z(II494) );
  NOR2 gate101 ( .A(II500_1), .B(II500_2), .Z(II484) );
  NAND2 gate100 ( .A(II368), .B(II307_1), .Z(II409) );
  NAND2 gate99 ( .A(II406), .B(II370), .Z(II378) );
  NOR2 gate98 ( .A(II409), .B(Y_5), .Z(II406) );
  NAND2 gate97 ( .A(X), .B(II309), .Z(II350) );
  NOR2 gate96 ( .A(Y_2), .B(II350), .Z(II347) );
  OR2 gate95 ( .A(Y_6), .B(II252), .Z(II287_2) );
  NAND2 gate94 ( .A(Y_5), .B(II265), .Z(II252) );
  OR2 gate93 ( .A(II195), .B(II256), .Z(II287_1) );
  OR2 gate92 ( .A(Y_2), .B(II100), .Z(II135_2) );
  NAND2 gate91 ( .A(Y_1), .B(II113), .Z(II100) );
  OR2 gate90 ( .A(II43), .B(II104), .Z(II135_1) );
  AND2 gate89 ( .A(P_7), .B(C_7), .Z(II512_2) );
  NOR2 gate88 ( .A(II371), .B(II378), .Z(P_7) );
  AND2 gate87 ( .A(P_6), .B(C_6), .Z(II512_1) );
  AND2 gate86 ( .A(P_1), .B(C_1), .Z(II508_2) );
  AND2 gate85 ( .A(X), .B(C_0), .Z(II508_1) );
  AND2 gate84 ( .A(P_4), .B(C_4), .Z(II504_2) );
  AND2 gate83 ( .A(P_3), .B(C_3), .Z(II504_1) );
  AND2 gate82 ( .A(P_2), .B(C_2), .Z(II500_2) );
  NOR2 gate81 ( .A(II310), .B(II350), .Z(P_2) );
  AND2 gate80 ( .A(P_5), .B(C_5), .Z(II500_1) );
  NOR2 gate79 ( .A(II369), .B(II409), .Z(P_5) );
  AND2 gate78 ( .A(P_8), .B(C_8), .Z(II497_1) );
  NOR3 gate77 ( .A(Y_7), .B(II378), .C(II372), .Z(P_8) );
  AND2 gate76 ( .A(II194), .B(II261), .Z(II283_2) );
  AND3 gate75 ( .A(II265), .B(II244), .C(Y_7), .Z(II283_1) );
  AND3 gate74 ( .A(II248), .B(II265), .C(Y_8), .Z(II279_2) );
  NOR2 gate73 ( .A(Clear), .B(II192), .Z(II265) );
  NAND2 gate72 ( .A(Y_7), .B(II202), .Z(II248) );
  AND3 gate71 ( .A(II193), .B(Y_7), .C(II261), .Z(II279_1) );
  NOR2 gate70 ( .A(II195), .B(II252), .Z(II261) );
  AND2 gate69 ( .A(II42), .B(II109), .Z(II131_2) );
  AND3 gate68 ( .A(II113), .B(II92), .C(Y_3), .Z(II131_1) );
  AND3 gate67 ( .A(II96), .B(II113), .C(Y_4), .Z(II127_2) );
  NOR2 gate66 ( .A(Clear), .B(II40), .Z(II113) );
  NAND2 gate65 ( .A(Y_3), .B(II50), .Z(II96) );
  AND3 gate64 ( .A(II41), .B(Y_3), .C(II109), .Z(II127_1) );
  NOR2 gate63 ( .A(II43), .B(II100), .Z(II109) );
  INV gate62 ( .A(II490), .Z(II489) );
  NOR2 gate61 ( .A(II508_1), .B(II508_2), .Z(II490) );
  INV gate60 ( .A(II488), .Z(II487) );
  NOR2 gate59 ( .A(II504_1), .B(II504_2), .Z(II488) );
  NOR3 gate58 ( .A(II495), .B(II487), .C(II497_1), .Z(II446) );
  INV gate57 ( .A(II374), .Z(P_6) );
  NAND2 gate56 ( .A(Y_6), .B(II406), .Z(II374) );
  INV gate55 ( .A(Y_8), .Z(II372) );
  INV gate54 ( .A(Y_7), .Z(II371) );
  INV gate53 ( .A(Y_6), .Z(II370) );
  INV gate52 ( .A(Y_5), .Z(II369) );
  INV gate51 ( .A(Y_4), .Z(II368) );
  INV gate50 ( .A(II317), .Z(P_4) );
  NAND2 gate49 ( .A(Y_4), .B(II318), .Z(II317) );
  INV gate48 ( .A(II316), .Z(P_3) );
  NAND2 gate47 ( .A(Y_3), .B(II347), .Z(II316) );
  INV gate46 ( .A(II314), .Z(P_1) );
  NAND2 gate45 ( .A(X), .B(Y_1), .Z(II314) );
  INV gate44 ( .A(Y_3), .Z(II311) );
  INV gate43 ( .A(Y_2), .Z(II310) );
  INV gate42 ( .A(Y_1), .Z(II309) );
  INV gate41 ( .A(II341), .Z(II318) );
  INV gate40 ( .A(II341), .Z(II307_1) );
  NAND2 gate39 ( .A(II311), .B(II347), .Z(II341) );
  NOR2 gate38 ( .A(II283_1), .B(II283_2), .Z(II199) );
  NOR2 gate37 ( .A(II279_1), .B(II279_2), .Z(II198) );
  INV gate36 ( .A(Y_5), .Z(II196) );
  INV gate35 ( .A(Y_6), .Z(II195) );
  INV gate34 ( .A(Y_7), .Z(II194) );
  INV gate33 ( .A(Y_8), .Z(II193) );
  INV gate32 ( .A(II1_1), .Z(II192) );
  NOR2 gate31 ( .A(II41), .B(II96), .Z(II1_1) );
  INV gate30 ( .A(II244), .Z(II202) );
  NAND2 gate29 ( .A(Y_6), .B(Y_5), .Z(II244) );
  NAND2 gate28 ( .A(II196), .B(II265), .Z(II256) );
  NOR2 gate27 ( .A(II131_1), .B(II131_2), .Z(II47) );
  NOR2 gate26 ( .A(II127_1), .B(II127_2), .Z(II46) );
  INV gate25 ( .A(Y_1), .Z(II44) );
  INV gate24 ( .A(Y_2), .Z(II43) );
  INV gate23 ( .A(Y_3), .Z(II42) );
  INV gate22 ( .A(Y_4), .Z(II41) );
  INV gate21 ( .A(X), .Z(II40) );
  INV gate20 ( .A(II92), .Z(II50) );
  NAND2 gate19 ( .A(Y_2), .B(Y_1), .Z(II92) );
  NAND2 gate18 ( .A(II44), .B(II113), .Z(II104) );
  INV gate16 ( .A(II256), .Z(II158) );
  NAND2 gate14 ( .A(II287_1), .B(II287_2), .Z(II157) );
  INV gate12 ( .A(II199), .Z(II156) );
  INV gate10 ( .A(II198), .Z(II155) );
  INV gate8 ( .A(II104), .Z(II6) );
  NAND2 gate6 ( .A(II135_1), .B(II135_2), .Z(II5) );
  INV gate4 ( .A(II47), .Z(II4) );
  INV gate2 ( .A(II46), .Z(II3) );
  INV gate1 ( .A(II446), .Z(Z) );
  NOR2 gate0 ( .A(II193), .B(II248), .Z(W) );
endmodule
