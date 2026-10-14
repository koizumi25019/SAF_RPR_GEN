
module s386_C ( v9, v8, v7, v6, v5, v4, v3, v2, v13_D_9, v13_D_8, v13_D_7, 
        v13_D_6, v13_D_5, v13_D_4, v13_D_3, v13_D_2, v13_D_12, v13_D_11, 
        v13_D_10, v13_D_1, v13_D_0, v12, v11, v10, v1, v0 );
  input v9, v8, v7, v6, v5, v4, v3, v2, v12, v11, v10, v1, v0;
  output v13_D_9, v13_D_8, v13_D_7, v13_D_6, v13_D_5, v13_D_4, v13_D_3,
         v13_D_2, v13_D_12, v13_D_11, v13_D_10, v13_D_1, v13_D_0;
  wire   B14B, B14Bbar, B15B, B16B, B17B, B18B, B19B, B20B, B21B, B22B, B23B,
         B24B, B25B, B26B, B27B, B28B, B29B, B30B, B31B, B32B, B33B, B34B,
         B34Bbar, B35B, B35Bbar, B36B, B37B, B38B, B39B, B40B, B41B, B42B,
         B43B, B44B, B45B, II104, II124, II148, II158, II164, II167, II171,
         II175, II186, II192, II195, II198, II201, II204, II207, II210, II213,
         II216, II219, II222, II225, II228, II231, II234, II64, II65, II89,
         II97, II98, IIII100, IIII102, IIII103, IIII105, IIII106, IIII108,
         IIII109, IIII111, IIII113, IIII114, IIII17, IIII18, IIII21, IIII22,
         IIII24, IIII25, IIII27, IIII28, IIII30, IIII31, IIII35, IIII36,
         IIII39, IIII40, IIII41, IIII43, IIII44, IIII47, IIII48, IIII50,
         IIII51, IIII53, IIII54, IIII56, IIII57, IIII59, IIII60, IIII62,
         IIII63, IIII65, IIII66, IIII69, IIII71, IIII73, IIII74, IIII76,
         IIII77, IIII79, IIII84, IIII85, IIII87, IIII90, IIII91, IIII93,
         IIII94, IIII96, IIII98, Lv13_D_0, Lv13_D_1, Lv13_D_10, Lv13_D_11,
         Lv13_D_12, Lv13_D_2, Lv13_D_3, Lv13_D_4, Lv13_D_5, Lv13_D_6, Lv13_D_7,
         Lv13_D_8, Lv13_D_9, v0bar, v10bar, v11bar, v12bar, v1bar, v3bar,
         v4bar, v5bar, v6bar, v7bar, v8bar, v9bar;

  AND3 gate164 ( .A(B20B), .B(v0), .C(v1bar), .Z(II195) );
  OR2 gate163 ( .A(IIII21), .B(IIII22), .Z(B20B) );
  AND3 gate162 ( .A(B37B), .B(v0), .C(v1bar), .Z(II192) );
  OR2 gate161 ( .A(IIII30), .B(IIII31), .Z(B37B) );
  AND2 gate160 ( .A(B45B), .B(v9bar), .Z(IIII17) );
  OR2 gate159 ( .A(IIII27), .B(IIII28), .Z(B45B) );
  AND3 gate158 ( .A(v0bar), .B(v10bar), .C(B41B), .Z(IIII18) );
  OR2 gate157 ( .A(IIII113), .B(IIII114), .Z(B41B) );
  AND3 gate156 ( .A(B31B), .B(v0), .C(v1bar), .Z(II186) );
  OR2 gate155 ( .A(IIII43), .B(IIII44), .Z(B31B) );
  AND2 gate154 ( .A(B19B), .B(v12bar), .Z(IIII21) );
  OR3 gate153 ( .A(IIII39), .B(IIII40), .C(IIII41), .Z(B19B) );
  AND2 gate152 ( .A(v7bar), .B(B18B), .Z(IIII22) );
  OR2 gate151 ( .A(IIII102), .B(IIII103), .Z(B18B) );
  AND2 gate150 ( .A(B24B), .B(v1), .Z(IIII24) );
  OR2 gate149 ( .A(IIII62), .B(IIII63), .Z(B24B) );
  AND2 gate148 ( .A(v0bar), .B(B22B), .Z(IIII25) );
  OR2 gate147 ( .A(IIII50), .B(IIII51), .Z(B22B) );
  AND2 gate146 ( .A(B44B), .B(v10bar), .Z(IIII27) );
  OR2 gate145 ( .A(IIII59), .B(IIII60), .Z(B44B) );
  AND4 gate144 ( .A(v10), .B(v11bar), .C(v12bar), .D(II175), .Z(IIII28) );
  AND3 gate143 ( .A(v0), .B(v7bar), .C(v8bar), .Z(II175) );
  AND3 gate142 ( .A(v11), .B(v12), .C(II171), .Z(IIII30) );
  AND3 gate141 ( .A(v5), .B(v7bar), .C(v8bar), .Z(II171) );
  AND3 gate140 ( .A(B36B), .B(v11bar), .C(v12bar), .Z(IIII31) );
  OR2 gate139 ( .A(IIII65), .B(IIII66), .Z(B36B) );
  AND3 gate138 ( .A(B33B), .B(v0), .C(v1bar), .Z(II167) );
  OR2 gate137 ( .A(IIII56), .B(IIII57), .Z(B33B) );
  AND3 gate136 ( .A(B15B), .B(v0), .C(v1bar), .Z(II164) );
  OR2 gate135 ( .A(IIII47), .B(IIII48), .Z(B15B) );
  AND2 gate134 ( .A(B28B), .B(v12bar), .Z(IIII35) );
  OR2 gate133 ( .A(IIII53), .B(IIII54), .Z(B28B) );
  AND4 gate132 ( .A(v7bar), .B(v8bar), .C(B25B), .D(B26B), .Z(IIII36) );
  OR2 gate131 ( .A(v0bar), .B(IIII96), .Z(B26B) );
  OR2 gate130 ( .A(v10bar), .B(IIII79), .Z(B25B) );
  AND3 gate129 ( .A(B39B), .B(v7bar), .C(v9bar), .Z(II158) );
  OR2 gate128 ( .A(IIII76), .B(IIII77), .Z(B39B) );
  AND4 gate127 ( .A(v5), .B(v7), .C(v8bar), .D(v11), .Z(IIII39) );
  AND3 gate126 ( .A(v3), .B(v8), .C(B16B), .Z(IIII40) );
  OR2 gate125 ( .A(B35Bbar), .B(IIII69), .Z(B16B) );
  AND3 gate124 ( .A(v4), .B(v11bar), .C(B17B), .Z(IIII41) );
  OR2 gate123 ( .A(v7), .B(IIII100), .Z(B17B) );
  AND2 gate122 ( .A(B30B), .B(v12bar), .Z(IIII43) );
  OR2 gate121 ( .A(IIII71), .B(v7), .Z(B30B) );
  AND2 gate120 ( .A(v8bar), .B(B29B), .Z(IIII44) );
  OR2 gate119 ( .A(IIII105), .B(IIII106), .Z(B29B) );
  AND3 gate118 ( .A(B38B), .B(v0), .C(v1bar), .Z(II148) );
  OR2 gate117 ( .A(IIII73), .B(IIII74), .Z(B38B) );
  AND3 gate116 ( .A(v4bar), .B(v11bar), .C(B34Bbar), .Z(IIII47) );
  AND2 gate115 ( .A(B14B), .B(v11), .Z(IIII48) );
  AND3 gate114 ( .A(B21B), .B(v7bar), .C(v8bar), .Z(IIII50) );
  OR2 gate113 ( .A(v10bar), .B(IIII87), .Z(B21B) );
  AND3 gate112 ( .A(v9bar), .B(v10bar), .C(v12bar), .Z(IIII51) );
  AND2 gate111 ( .A(B27B), .B(v1), .Z(IIII53) );
  OR2 gate110 ( .A(IIII93), .B(IIII94), .Z(B27B) );
  AND3 gate109 ( .A(v0bar), .B(v9bar), .C(v10bar), .Z(IIII54) );
  AND3 gate108 ( .A(v11), .B(v12bar), .C(B14Bbar), .Z(IIII56) );
  AND2 gate107 ( .A(B32B), .B(v7bar), .Z(IIII57) );
  OR2 gate106 ( .A(IIII84), .B(IIII85), .Z(B32B) );
  AND3 gate105 ( .A(B43B), .B(v8), .C(v12bar), .Z(IIII59) );
  OR2 gate104 ( .A(IIII108), .B(IIII109), .Z(B43B) );
  AND2 gate103 ( .A(v1), .B(B42B), .Z(IIII60) );
  OR2 gate102 ( .A(IIII111), .B(v12bar), .Z(B42B) );
  AND3 gate101 ( .A(B23B), .B(v7bar), .C(v8bar), .Z(IIII62) );
  OR2 gate100 ( .A(IIII90), .B(IIII91), .Z(B23B) );
  AND3 gate99 ( .A(v9bar), .B(v10bar), .C(v12bar), .Z(IIII63) );
  AND2 gate98 ( .A(B35B), .B(B34B), .Z(IIII65) );
  AND2 gate97 ( .A(v4), .B(v7), .Z(IIII66) );
  AND4 gate96 ( .A(B40B), .B(v1), .C(v7bar), .D(v8bar), .Z(II124) );
  OR2 gate95 ( .A(IIII98), .B(v10bar), .Z(B40B) );
  AND2 gate94 ( .A(v7), .B(v11bar), .Z(IIII69) );
  AND3 gate93 ( .A(v4bar), .B(v11bar), .C(B34Bbar), .Z(IIII71) );
  AND3 gate92 ( .A(v4bar), .B(v11bar), .C(B34Bbar), .Z(IIII73) );
  AND3 gate91 ( .A(v7), .B(v8bar), .C(v11), .Z(IIII74) );
  AND4 gate90 ( .A(v1bar), .B(v4), .C(v10bar), .D(B34Bbar), .Z(IIII76) );
  AND3 gate89 ( .A(v0), .B(v8bar), .C(v10), .Z(IIII77) );
  AND2 gate88 ( .A(v11bar), .B(v12bar), .Z(IIII79) );
  AND3 gate87 ( .A(v8bar), .B(v11), .C(v12), .Z(IIII84) );
  AND3 gate86 ( .A(v11bar), .B(v12bar), .C(II104), .Z(IIII85) );
  AND3 gate85 ( .A(v2), .B(v3), .C(v8), .Z(II104) );
  AND4 gate84 ( .A(v5bar), .B(v9), .C(v11bar), .D(v12bar), .Z(IIII87) );
  AND4 gate83 ( .A(v9bar), .B(v10), .C(v11bar), .D(v12bar), .Z(II98) );
  AND4 gate82 ( .A(v0), .B(v6bar), .C(v7bar), .D(v8bar), .Z(II97) );
  AND2 gate81 ( .A(v9bar), .B(v10bar), .Z(IIII90) );
  AND3 gate80 ( .A(v0), .B(v11bar), .C(v12bar), .Z(IIII91) );
  AND2 gate79 ( .A(v9bar), .B(v10bar), .Z(IIII93) );
  AND3 gate78 ( .A(v10), .B(v11bar), .C(II89), .Z(IIII94) );
  AND3 gate77 ( .A(v5bar), .B(v7bar), .C(v8bar), .Z(II89) );
  AND2 gate76 ( .A(v1), .B(v9bar), .Z(IIII96) );
  AND2 gate75 ( .A(v0), .B(v5), .Z(IIII98) );
  AND2 gate74 ( .A(v2), .B(v8bar), .Z(IIII100) );
  AND3 gate73 ( .A(v8bar), .B(v11bar), .C(v12), .Z(IIII102) );
  AND3 gate72 ( .A(v8), .B(v11), .C(v12bar), .Z(IIII103) );
  AND3 gate71 ( .A(v2), .B(v11bar), .C(v12bar), .Z(IIII105) );
  AND4 gate70 ( .A(v5bar), .B(v7bar), .C(v11), .D(v12), .Z(IIII106) );
  AND2 gate69 ( .A(v7), .B(v11), .Z(IIII108) );
  AND3 gate68 ( .A(v3bar), .B(v4bar), .C(v11bar), .Z(IIII109) );
  AND2 gate67 ( .A(v7bar), .B(v8bar), .Z(IIII111) );
  AND2 gate66 ( .A(v7bar), .B(v8bar), .Z(IIII113) );
  AND2 gate65 ( .A(v9bar), .B(v12bar), .Z(IIII114) );
  AND4 gate64 ( .A(v9), .B(v10), .C(v11bar), .D(v12bar), .Z(II65) );
  AND4 gate63 ( .A(v0bar), .B(v5), .C(v7bar), .D(v8bar), .Z(II64) );
  INV gate62 ( .A(Lv13_D_0), .Z(II234) );
  AND4 gate61 ( .A(v9bar), .B(v10bar), .C(v12bar), .D(II164), .Z(Lv13_D_0) );
  INV gate60 ( .A(Lv13_D_1), .Z(II231) );
  AND3 gate59 ( .A(v9bar), .B(v10bar), .C(II195), .Z(Lv13_D_1) );
  INV gate58 ( .A(Lv13_D_2), .Z(II228) );
  OR2 gate57 ( .A(IIII24), .B(IIII25), .Z(Lv13_D_2) );
  INV gate56 ( .A(Lv13_D_3), .Z(II225) );
  OR2 gate55 ( .A(IIII35), .B(IIII36), .Z(Lv13_D_3) );
  INV gate54 ( .A(Lv13_D_4), .Z(II222) );
  AND3 gate53 ( .A(v9bar), .B(v10bar), .C(II186), .Z(Lv13_D_4) );
  INV gate52 ( .A(Lv13_D_5), .Z(II219) );
  AND3 gate51 ( .A(v9bar), .B(v10bar), .C(II167), .Z(Lv13_D_5) );
  INV gate50 ( .A(Lv13_D_6), .Z(II216) );
  AND3 gate49 ( .A(v9bar), .B(v10bar), .C(II192), .Z(Lv13_D_6) );
  INV gate48 ( .A(Lv13_D_7), .Z(II213) );
  AND4 gate47 ( .A(v9bar), .B(v10bar), .C(v12bar), .D(II148), .Z(Lv13_D_7) );
  INV gate46 ( .A(Lv13_D_8), .Z(II210) );
  AND2 gate45 ( .A(II97), .B(II98), .Z(Lv13_D_8) );
  INV gate44 ( .A(Lv13_D_9), .Z(II207) );
  AND3 gate43 ( .A(v11bar), .B(v12bar), .C(II158), .Z(Lv13_D_9) );
  INV gate42 ( .A(Lv13_D_10), .Z(II204) );
  AND4 gate41 ( .A(v9), .B(v11bar), .C(v12bar), .D(II124), .Z(Lv13_D_10) );
  INV gate40 ( .A(Lv13_D_11), .Z(II201) );
  OR2 gate39 ( .A(IIII17), .B(IIII18), .Z(Lv13_D_11) );
  INV gate38 ( .A(Lv13_D_12), .Z(II198) );
  AND2 gate37 ( .A(II64), .B(II65), .Z(Lv13_D_12) );
  INV gate36 ( .A(v1), .Z(v1bar) );
  INV gate35 ( .A(v9), .Z(v9bar) );
  INV gate34 ( .A(v10), .Z(v10bar) );
  INV gate33 ( .A(v0), .Z(v0bar) );
  INV gate32 ( .A(v12), .Z(v12bar) );
  INV gate31 ( .A(v7), .Z(v7bar) );
  INV gate30 ( .A(v8), .Z(v8bar) );
  INV gate29 ( .A(v11), .Z(v11bar) );
  INV gate28 ( .A(v4), .Z(v4bar) );
  INV gate27 ( .A(B34B), .Z(B34Bbar) );
  OR2 gate26 ( .A(v8bar), .B(v3), .Z(B34B) );
  INV gate25 ( .A(B14B), .Z(B14Bbar) );
  OR2 gate24 ( .A(v7bar), .B(v8bar), .Z(B14B) );
  INV gate23 ( .A(B35B), .Z(B35Bbar) );
  OR2 gate22 ( .A(v2), .B(v7), .Z(B35B) );
  INV gate21 ( .A(v5), .Z(v5bar) );
  INV gate20 ( .A(v6), .Z(v6bar) );
  INV gate19 ( .A(v3), .Z(v3bar) );
  INV gate17 ( .A(II234), .Z(v13_D_0) );
  INV gate15 ( .A(II231), .Z(v13_D_1) );
  INV gate13 ( .A(II228), .Z(v13_D_2) );
  INV gate11 ( .A(II225), .Z(v13_D_3) );
  INV gate9 ( .A(II222), .Z(v13_D_4) );
  INV gate7 ( .A(II219), .Z(v13_D_5) );
  INV gate6 ( .A(II216), .Z(v13_D_6) );
  INV gate5 ( .A(II213), .Z(v13_D_7) );
  INV gate4 ( .A(II210), .Z(v13_D_8) );
  INV gate3 ( .A(II207), .Z(v13_D_9) );
  INV gate2 ( .A(II204), .Z(v13_D_10) );
  INV gate1 ( .A(II201), .Z(v13_D_11) );
  INV gate0 ( .A(II198), .Z(v13_D_12) );
endmodule
