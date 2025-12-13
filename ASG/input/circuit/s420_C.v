
module s420_C ( Z, Y_9, Y_8, Y_7, Y_6, Y_5, Y_4, Y_3, Y_2, Y_16, Y_15, Y_14, 
        Y_13, Y_12, Y_11, Y_10, Y_1, X, W, II6, II5, II462, II461, II460, 
        II459, II4, II310, II309, II308, II307, II3, II158, II157, II156, 
        II155, Clear, C_9, C_8, C_7, C_6, C_5, C_4, C_3, C_2, C_16, C_15, C_14, 
        C_13, C_12, C_11, C_10, C_1, C_0 );
  input Y_9, Y_8, Y_7, Y_6, Y_5, Y_4, Y_3, Y_2, Y_16, Y_15, Y_14, Y_13, Y_12,
         Y_11, Y_10, Y_1, X, Clear, C_9, C_8, C_7, C_6, C_5, C_4, C_3, C_2,
         C_16, C_15, C_14, C_13, C_12, C_11, C_10, C_1, C_0;
  output Z, W, II6, II5, II462, II461, II460, II459, II4, II310, II309, II308,
         II307, II3, II158, II157, II156, II155;
  wire   II100, II1001_1, II1008_1, II1008_2, II1012_1, II1012_2, II1016_1,
         II1019_1, II1019_2, II1023_1, II1023_2, II1028_1, II1032_1, II1032_2,
         II104, II109, II113, II127_1, II127_2, II131_1, II131_2, II135_1,
         II135_2, II192, II193, II194, II195, II196, II198, II199, II1_1,
         II1_2, II1_3, II202, II244, II248, II252, II256, II261, II265,
         II279_1, II279_2, II283_1, II283_2, II287_1, II287_2, II344, II345,
         II346, II347, II348, II350, II351, II354, II396, II40, II400, II404,
         II408, II41, II413, II417, II42, II43, II431_1, II431_2, II435_1,
         II435_2, II439_1, II439_2, II44, II46, II47, II496, II497, II498,
         II499, II50, II500, II502, II503, II506, II548, II552, II556, II560,
         II565, II569, II583_1, II583_2, II587_1, II587_2, II591_1, II591_2,
         II611_1, II611_2, II611_3, II613, II614, II615, II618, II620, II621,
         II622, II645, II651, II654, II672, II673, II674, II675, II680, II681,
         II682, II708, II713, II716, II720, II721, II740, II741, II742, II743,
         II748, II749, II750, II776, II781, II784, II788, II789, II808, II809,
         II810, II811, II812, II814, II818, II846, II849, II884, II901, II902,
         II92, II945, II949, II957, II96, II961, II972, II974, II975, II978,
         II980, II981, II982, II984, II989, II990, II991, II992, II997, P_1,
         P_10, P_11, P_12, P_13, P_14, P_15, P_16, P_2, P_3, P_4, P_5, P_6,
         P_7, P_8, P_9;

  NAND4 gate211 ( .A(II990), .B(II984), .C(II945), .D(II961), .Z(II997) );
  NOR2 gate210 ( .A(II975), .B(II1028_1), .Z(II984) );
  NOR2 gate209 ( .A(II1012_1), .B(II1012_2), .Z(II974) );
  NOR2 gate208 ( .A(II1023_1), .B(II1023_2), .Z(II980) );
  NOR2 gate207 ( .A(II1019_1), .B(II1019_2), .Z(II978) );
  NAND2 gate206 ( .A(II1016_1), .B(II972), .Z(II975) );
  NOR2 gate205 ( .A(II1008_1), .B(II1008_2), .Z(II972) );
  NAND2 gate204 ( .A(P_14), .B(C_14), .Z(II961) );
  NAND2 gate203 ( .A(P_13), .B(C_13), .Z(II957) );
  NOR2 gate202 ( .A(II809), .B(II849), .Z(P_13) );
  NAND2 gate201 ( .A(P_11), .B(C_11), .Z(II949) );
  NAND2 gate200 ( .A(P_10), .B(C_10), .Z(II945) );
  NOR2 gate199 ( .A(II742), .B(II784), .Z(P_10) );
  NAND2 gate198 ( .A(II808), .B(II611_3), .Z(II849) );
  NAND2 gate197 ( .A(II846), .B(II810), .Z(II818) );
  NOR2 gate196 ( .A(II849), .B(Y_13), .Z(II846) );
  NAND2 gate195 ( .A(II741), .B(II789), .Z(II784) );
  NOR2 gate194 ( .A(Y_10), .B(II784), .Z(II781) );
  NAND2 gate193 ( .A(II673), .B(II721), .Z(II716) );
  NOR2 gate192 ( .A(Y_6), .B(II716), .Z(II713) );
  NAND2 gate191 ( .A(X), .B(II613), .Z(II654) );
  NOR2 gate190 ( .A(Y_2), .B(II654), .Z(II651) );
  OR2 gate189 ( .A(II884), .B(II901), .Z(II1016_1) );
  OR2 gate188 ( .A(Y_14), .B(II556), .Z(II591_2) );
  NAND2 gate187 ( .A(Y_13), .B(II569), .Z(II556) );
  OR2 gate186 ( .A(II499), .B(II560), .Z(II591_1) );
  OR2 gate185 ( .A(Y_10), .B(II404), .Z(II439_2) );
  NAND2 gate184 ( .A(Y_9), .B(II417), .Z(II404) );
  OR2 gate183 ( .A(II347), .B(II408), .Z(II439_1) );
  OR2 gate182 ( .A(Y_6), .B(II252), .Z(II287_2) );
  NAND2 gate181 ( .A(Y_5), .B(II265), .Z(II252) );
  OR2 gate180 ( .A(II195), .B(II256), .Z(II287_1) );
  OR2 gate179 ( .A(Y_2), .B(II100), .Z(II135_2) );
  NAND2 gate178 ( .A(Y_1), .B(II113), .Z(II100) );
  OR2 gate177 ( .A(II43), .B(II104), .Z(II135_1) );
  AND2 gate176 ( .A(P_1), .B(C_1), .Z(II1032_2) );
  AND2 gate175 ( .A(P_2), .B(C_2), .Z(II1032_1) );
  NOR2 gate174 ( .A(II614), .B(II654), .Z(P_2) );
  AND2 gate173 ( .A(P_5), .B(C_5), .Z(II1028_1) );
  NOR2 gate172 ( .A(II673), .B(II720), .Z(P_5) );
  AND2 gate171 ( .A(P_4), .B(C_4), .Z(II1023_2) );
  AND2 gate170 ( .A(P_8), .B(C_8), .Z(II1023_1) );
  AND2 gate169 ( .A(P_6), .B(C_6), .Z(II1019_2) );
  NOR2 gate168 ( .A(II674), .B(II716), .Z(P_6) );
  AND2 gate167 ( .A(P_7), .B(C_7), .Z(II1019_1) );
  AND2 gate166 ( .A(P_16), .B(C_16), .Z(II1012_2) );
  NOR3 gate165 ( .A(Y_15), .B(II818), .C(II812), .Z(P_16) );
  AND2 gate164 ( .A(P_15), .B(C_15), .Z(II1012_1) );
  NOR2 gate163 ( .A(II811), .B(II818), .Z(P_15) );
  AND2 gate162 ( .A(P_12), .B(C_12), .Z(II1008_2) );
  AND2 gate161 ( .A(P_3), .B(C_3), .Z(II1008_1) );
  AND2 gate160 ( .A(P_9), .B(C_9), .Z(II1001_1) );
  NOR2 gate159 ( .A(II741), .B(II788), .Z(P_9) );
  AND2 gate158 ( .A(II498), .B(II565), .Z(II587_2) );
  AND3 gate157 ( .A(II569), .B(II548), .C(Y_15), .Z(II587_1) );
  AND3 gate156 ( .A(II552), .B(II569), .C(Y_16), .Z(II583_2) );
  NOR2 gate155 ( .A(Clear), .B(II496), .Z(II569) );
  NAND2 gate154 ( .A(Y_15), .B(II506), .Z(II552) );
  AND3 gate153 ( .A(II497), .B(Y_15), .C(II565), .Z(II583_1) );
  NOR2 gate152 ( .A(II499), .B(II556), .Z(II565) );
  AND2 gate151 ( .A(II346), .B(II413), .Z(II435_2) );
  AND3 gate150 ( .A(II417), .B(II396), .C(Y_11), .Z(II435_1) );
  AND3 gate149 ( .A(II400), .B(II417), .C(Y_12), .Z(II431_2) );
  NOR2 gate148 ( .A(Clear), .B(II344), .Z(II417) );
  NAND2 gate147 ( .A(Y_11), .B(II354), .Z(II400) );
  AND3 gate146 ( .A(II345), .B(Y_11), .C(II413), .Z(II431_1) );
  NOR2 gate145 ( .A(II347), .B(II404), .Z(II413) );
  AND2 gate144 ( .A(II194), .B(II261), .Z(II283_2) );
  AND3 gate143 ( .A(II265), .B(II244), .C(Y_7), .Z(II283_1) );
  AND3 gate142 ( .A(II248), .B(II265), .C(Y_8), .Z(II279_2) );
  NOR2 gate141 ( .A(Clear), .B(II192), .Z(II265) );
  NAND2 gate140 ( .A(Y_7), .B(II202), .Z(II248) );
  AND3 gate139 ( .A(II193), .B(Y_7), .C(II261), .Z(II279_1) );
  NOR2 gate138 ( .A(II195), .B(II252), .Z(II261) );
  AND2 gate137 ( .A(II42), .B(II109), .Z(II131_2) );
  AND3 gate136 ( .A(II113), .B(II92), .C(Y_3), .Z(II131_1) );
  AND3 gate135 ( .A(II96), .B(II113), .C(Y_4), .Z(II127_2) );
  NOR2 gate134 ( .A(Clear), .B(II40), .Z(II113) );
  NAND2 gate133 ( .A(Y_3), .B(II50), .Z(II96) );
  AND3 gate132 ( .A(II41), .B(Y_3), .C(II109), .Z(II127_1) );
  NOR2 gate131 ( .A(II43), .B(II100), .Z(II109) );
  INV gate130 ( .A(II992), .Z(II991) );
  NOR2 gate129 ( .A(II1032_1), .B(II1032_2), .Z(II992) );
  INV gate128 ( .A(II989), .Z(II990) );
  NAND4 gate127 ( .A(II982), .B(II949), .C(II974), .D(II957), .Z(II989) );
  INV gate126 ( .A(II981), .Z(II982) );
  NAND2 gate125 ( .A(II978), .B(II980), .Z(II981) );
  NOR3 gate124 ( .A(II997), .B(II991), .C(II1001_1), .Z(II902) );
  INV gate123 ( .A(C_0), .Z(II901) );
  INV gate122 ( .A(X), .Z(II884) );
  INV gate121 ( .A(II814), .Z(P_14) );
  NAND2 gate120 ( .A(Y_14), .B(II846), .Z(II814) );
  INV gate119 ( .A(Y_16), .Z(II812) );
  INV gate118 ( .A(Y_15), .Z(II811) );
  INV gate117 ( .A(Y_14), .Z(II810) );
  INV gate116 ( .A(Y_13), .Z(II809) );
  INV gate115 ( .A(Y_12), .Z(II808) );
  INV gate114 ( .A(II788), .Z(II789) );
  NAND2 gate113 ( .A(II611_2), .B(II740), .Z(II788) );
  INV gate112 ( .A(II749), .Z(P_12) );
  NAND2 gate111 ( .A(Y_12), .B(II750), .Z(II749) );
  INV gate110 ( .A(II748), .Z(P_11) );
  NAND2 gate109 ( .A(Y_11), .B(II781), .Z(II748) );
  INV gate108 ( .A(Y_11), .Z(II743) );
  INV gate107 ( .A(Y_10), .Z(II742) );
  INV gate106 ( .A(Y_9), .Z(II741) );
  INV gate105 ( .A(Y_8), .Z(II740) );
  INV gate104 ( .A(II776), .Z(II750) );
  INV gate103 ( .A(II776), .Z(II611_3) );
  NAND2 gate102 ( .A(II743), .B(II781), .Z(II776) );
  INV gate101 ( .A(II720), .Z(II721) );
  NAND2 gate100 ( .A(II611_1), .B(II672), .Z(II720) );
  INV gate99 ( .A(II681), .Z(P_8) );
  NAND2 gate98 ( .A(Y_8), .B(II682), .Z(II681) );
  INV gate97 ( .A(II680), .Z(P_7) );
  NAND2 gate96 ( .A(Y_7), .B(II713), .Z(II680) );
  INV gate95 ( .A(Y_7), .Z(II675) );
  INV gate94 ( .A(Y_6), .Z(II674) );
  INV gate93 ( .A(Y_5), .Z(II673) );
  INV gate92 ( .A(Y_4), .Z(II672) );
  INV gate91 ( .A(II708), .Z(II682) );
  INV gate90 ( .A(II708), .Z(II611_2) );
  NAND2 gate89 ( .A(II675), .B(II713), .Z(II708) );
  INV gate88 ( .A(II621), .Z(P_4) );
  NAND2 gate87 ( .A(Y_4), .B(II622), .Z(II621) );
  INV gate86 ( .A(II620), .Z(P_3) );
  NAND2 gate85 ( .A(Y_3), .B(II651), .Z(II620) );
  INV gate84 ( .A(II618), .Z(P_1) );
  NAND2 gate83 ( .A(X), .B(Y_1), .Z(II618) );
  INV gate82 ( .A(Y_3), .Z(II615) );
  INV gate81 ( .A(Y_2), .Z(II614) );
  INV gate80 ( .A(Y_1), .Z(II613) );
  INV gate79 ( .A(II645), .Z(II622) );
  INV gate78 ( .A(II645), .Z(II611_1) );
  NAND2 gate77 ( .A(II615), .B(II651), .Z(II645) );
  NOR2 gate76 ( .A(II587_1), .B(II587_2), .Z(II503) );
  NOR2 gate75 ( .A(II583_1), .B(II583_2), .Z(II502) );
  INV gate74 ( .A(Y_13), .Z(II500) );
  INV gate73 ( .A(Y_14), .Z(II499) );
  INV gate72 ( .A(Y_15), .Z(II498) );
  INV gate71 ( .A(Y_16), .Z(II497) );
  INV gate70 ( .A(II1_3), .Z(II496) );
  NOR2 gate69 ( .A(II345), .B(II400), .Z(II1_3) );
  INV gate68 ( .A(II548), .Z(II506) );
  NAND2 gate67 ( .A(Y_14), .B(Y_13), .Z(II548) );
  NAND2 gate66 ( .A(II500), .B(II569), .Z(II560) );
  NOR2 gate65 ( .A(II435_1), .B(II435_2), .Z(II351) );
  NOR2 gate64 ( .A(II431_1), .B(II431_2), .Z(II350) );
  INV gate63 ( .A(Y_9), .Z(II348) );
  INV gate62 ( .A(Y_10), .Z(II347) );
  INV gate61 ( .A(Y_11), .Z(II346) );
  INV gate60 ( .A(Y_12), .Z(II345) );
  INV gate59 ( .A(II1_2), .Z(II344) );
  NOR2 gate58 ( .A(II193), .B(II248), .Z(II1_2) );
  INV gate57 ( .A(II396), .Z(II354) );
  NAND2 gate56 ( .A(Y_10), .B(Y_9), .Z(II396) );
  NAND2 gate55 ( .A(II348), .B(II417), .Z(II408) );
  NOR2 gate54 ( .A(II283_1), .B(II283_2), .Z(II199) );
  NOR2 gate53 ( .A(II279_1), .B(II279_2), .Z(II198) );
  INV gate52 ( .A(Y_5), .Z(II196) );
  INV gate51 ( .A(Y_6), .Z(II195) );
  INV gate50 ( .A(Y_7), .Z(II194) );
  INV gate49 ( .A(Y_8), .Z(II193) );
  INV gate48 ( .A(II1_1), .Z(II192) );
  NOR2 gate47 ( .A(II41), .B(II96), .Z(II1_1) );
  INV gate46 ( .A(II244), .Z(II202) );
  NAND2 gate45 ( .A(Y_6), .B(Y_5), .Z(II244) );
  NAND2 gate44 ( .A(II196), .B(II265), .Z(II256) );
  NOR2 gate43 ( .A(II131_1), .B(II131_2), .Z(II47) );
  NOR2 gate42 ( .A(II127_1), .B(II127_2), .Z(II46) );
  INV gate41 ( .A(Y_1), .Z(II44) );
  INV gate40 ( .A(Y_2), .Z(II43) );
  INV gate39 ( .A(Y_3), .Z(II42) );
  INV gate38 ( .A(Y_4), .Z(II41) );
  INV gate37 ( .A(X), .Z(II40) );
  INV gate36 ( .A(II92), .Z(II50) );
  NAND2 gate35 ( .A(Y_2), .B(Y_1), .Z(II92) );
  NAND2 gate34 ( .A(II44), .B(II113), .Z(II104) );
  INV gate32 ( .A(II560), .Z(II462) );
  NAND2 gate30 ( .A(II591_1), .B(II591_2), .Z(II461) );
  INV gate28 ( .A(II503), .Z(II460) );
  INV gate26 ( .A(II502), .Z(II459) );
  INV gate24 ( .A(II408), .Z(II310) );
  NAND2 gate22 ( .A(II439_1), .B(II439_2), .Z(II309) );
  INV gate20 ( .A(II351), .Z(II308) );
  INV gate18 ( .A(II350), .Z(II307) );
  INV gate16 ( .A(II256), .Z(II158) );
  NAND2 gate14 ( .A(II287_1), .B(II287_2), .Z(II157) );
  INV gate12 ( .A(II199), .Z(II156) );
  INV gate10 ( .A(II198), .Z(II155) );
  INV gate8 ( .A(II104), .Z(II6) );
  NAND2 gate6 ( .A(II135_1), .B(II135_2), .Z(II5) );
  INV gate4 ( .A(II47), .Z(II4) );
  INV gate2 ( .A(II46), .Z(II3) );
  INV gate1 ( .A(II902), .Z(Z) );
  NOR2 gate0 ( .A(II497), .B(II552), .Z(W) );
endmodule
