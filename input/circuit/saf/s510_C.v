
module s510_C ( vsync, st_5, st_4, st_3, st_2, st_1, st_0, pcnt6, pcnt27, 
        pcnt241, pcnt17, pcnt12, pclr, pc, john, csync, csm, cnt591, cnt567, 
        cnt511, cnt509, cnt45, cnt44, cnt284, cnt283, cnt272, cnt261, cnt21, 
        cnt13, cnt10, cclr, cblank, II7, II6, II5, II4, II3, II2 );
  input st_5, st_4, st_3, st_2, st_1, st_0, pcnt6, pcnt27, pcnt241, pcnt17,
         pcnt12, john, cnt591, cnt567, cnt511, cnt509, cnt45, cnt44, cnt284,
         cnt283, cnt272, cnt261, cnt21, cnt13, cnt10;
  output vsync, pclr, pc, csync, csm, cclr, cblank, II7, II6, II5, II4, II3,
         II2;
  wire   II1038_1, II104, II1044_1, II1055_1, II1055_2, II1059_1, II1062_1,
         II1065_1, II1068_1, II1071_1, II1074_1, II1077_1, II1081_1, II1081_2,
         II1085_1, II1085_2, II1089_1, II1092_1, II1095_1, II1095_2, II1099_1,
         II1102_1, II1102_2, II1106_1, II1106_2, II1110_1, II1113_1, II1116_1,
         II1116_2, II1120_1, II1123_1, II130, II131, II204, II205, II207,
         II209, II213, II216, II217, II230, II232, II234, II259, II266, II270,
         II274, II278, II282, II298, II326, II346, II347, II371, II390, II455,
         II458, II462, II463, II466, II467, II474, II475, II478, II482, II483,
         II486, II487, II490, II494, II495, II498, II506, II511, II530, II531,
         II535, II539, II543, II546, II547, II551, II554, II555, II559, II56,
         II563, II566, II567, II57, II570, II574, II578, II58, II583, II587,
         II59, II590, II591, II594, II595, II598, II60, II603, II606, II607,
         II61, II615, II618, II627, II638, II642, II658, II663, II666, II667,
         II67, II671, II675, II68, II69, II694, II698, II70, II710, II714,
         II73, II731, II739, II742, II747, II774, II778, II779, II78, II782,
         II787, II795, II798, II799, II810, II814, II821, II823, II827, II831,
         II834, II837, II838, II841, II855, II861, II863, II867, II870, II872,
         II874, II877, II881, II884, II887, II889, II895, II899, II900_1,
         II903_1, II903_2, II909_1, II914_1, II917_1, II917_2, II921_1,
         II924_1, II924_2, II928_1, II933_1, II936_1, II936_2, II940_1,
         II943_1, II946_1, II946_2, II95, II950_1, II950_2, II954_1, II954_2,
         II958_1, II958_2, II962_1, II962_2, II967_1, II970_1, II975_1,
         II978_1, II982_1, II985_1, II988_1;

  NOR2 gate216 ( .A(II68), .B(II988_1), .Z(II371) );
  NOR2 gate215 ( .A(st_0), .B(II967_1), .Z(II259) );
  NOR2 gate214 ( .A(II874), .B(II1113_1), .Z(II887) );
  NAND3 gate213 ( .A(II1099_1), .B(II863), .C(II831), .Z(II874) );
  NOR2 gate212 ( .A(II371), .B(II1065_1), .Z(II831) );
  NOR2 gate211 ( .A(II1085_1), .B(II1085_2), .Z(II863) );
  NOR2 gate210 ( .A(II1055_1), .B(II1055_2), .Z(II821) );
  NOR2 gate209 ( .A(II884), .B(II1120_1), .Z(II895) );
  NOR3 gate208 ( .A(II872), .B(II347), .C(II1123_1), .Z(II899) );
  NOR3 gate207 ( .A(II1106_1), .B(II1106_2), .C(II838), .Z(II881) );
  NOR2 gate206 ( .A(II531), .B(II1062_1), .Z(II827) );
  NOR3 gate205 ( .A(II1116_1), .B(II1116_2), .C(II870), .Z(II889) );
  NAND3 gate204 ( .A(II1110_1), .B(II861), .C(II326), .Z(II884) );
  NOR2 gate203 ( .A(II1081_1), .B(II1081_2), .Z(II861) );
  NOR2 gate202 ( .A(II799), .B(II1077_1), .Z(II841) );
  NOR2 gate201 ( .A(II834), .B(II1089_1), .Z(II867) );
  NOR2 gate200 ( .A(II615), .B(st_3), .Z(II855) );
  NOR2 gate199 ( .A(II259), .B(II1059_1), .Z(II823) );
  NAND2 gate198 ( .A(II978_1), .B(st_1), .Z(II282) );
  NAND2 gate197 ( .A(II975_1), .B(II60), .Z(II278) );
  NOR2 gate196 ( .A(II950_1), .B(II950_2), .Z(II213) );
  NOR3 gate195 ( .A(II1102_1), .B(II1102_2), .C(II551), .Z(II877) );
  NAND2 gate194 ( .A(II970_1), .B(st_1), .Z(II266) );
  NAND3 gate193 ( .A(II1095_1), .B(II1095_2), .C(II774), .Z(II872) );
  NAND2 gate192 ( .A(II1074_1), .B(II530), .Z(II838) );
  NAND2 gate191 ( .A(st_3), .B(II615), .Z(II270) );
  NOR2 gate190 ( .A(II475), .B(st_2), .Z(II615) );
  NOR2 gate189 ( .A(II487), .B(II1071_1), .Z(II837) );
  NAND2 gate188 ( .A(II982_1), .B(II61), .Z(II326) );
  NAND2 gate187 ( .A(II1038_1), .B(II567), .Z(II714) );
  NAND2 gate186 ( .A(II467), .B(cnt10), .Z(II710) );
  NAND2 gate185 ( .A(II539), .B(II574), .Z(II298) );
  NAND2 gate184 ( .A(II1092_1), .B(II566), .Z(II870) );
  NAND3 gate183 ( .A(II67), .B(II559), .C(II675), .Z(II782) );
  NAND3 gate182 ( .A(pcnt6), .B(cnt284), .C(II455), .Z(II810) );
  NAND2 gate181 ( .A(II56), .B(II667), .Z(II274) );
  NOR2 gate180 ( .A(II578), .B(II56), .Z(II563) );
  NAND2 gate179 ( .A(II1068_1), .B(II642), .Z(II834) );
  NAND2 gate178 ( .A(st_0), .B(II583), .Z(II390) );
  NOR2 gate177 ( .A(II511), .B(II60), .Z(II583) );
  NAND3 gate176 ( .A(st_5), .B(II547), .C(II458), .Z(II774) );
  NOR2 gate175 ( .A(st_5), .B(st_1), .Z(II739) );
  NAND2 gate174 ( .A(II511), .B(st_0), .Z(II638) );
  NOR2 gate173 ( .A(st_3), .B(st_5), .Z(II511) );
  NAND2 gate172 ( .A(st_3), .B(II57), .Z(II574) );
  NAND3 gate171 ( .A(II58), .B(cnt21), .C(II595), .Z(II814) );
  NAND2 gate170 ( .A(st_2), .B(II58), .Z(II658) );
  OR2 gate169 ( .A(st_0), .B(II731), .Z(II924_1) );
  NOR2 gate168 ( .A(II583), .B(II607), .Z(II731) );
  OR2 gate167 ( .A(II61), .B(II530), .Z(II1110_1) );
  OR2 gate166 ( .A(st_0), .B(II530), .Z(II928_1) );
  OR2 gate165 ( .A(II483), .B(II747), .Z(II978_1) );
  NOR2 gate164 ( .A(II638), .B(II1044_1), .Z(II747) );
  OR2 gate163 ( .A(II531), .B(II483), .Z(II975_1) );
  OR2 gate162 ( .A(II462), .B(II73), .Z(II962_1) );
  OR2 gate161 ( .A(II606), .B(II742), .Z(II903_1) );
  NAND2 gate160 ( .A(II56), .B(st_0), .Z(II742) );
  OR2 gate159 ( .A(II56), .B(II207), .Z(II900_1) );
  NOR2 gate158 ( .A(II595), .B(II943_1), .Z(II207) );
  OR2 gate157 ( .A(II495), .B(II603), .Z(II970_1) );
  OR2 gate156 ( .A(II475), .B(II578), .Z(II1095_2) );
  OR2 gate155 ( .A(II475), .B(II546), .Z(II1074_1) );
  OR2 gate154 ( .A(II559), .B(II487), .Z(II982_1) );
  NOR2 gate153 ( .A(II658), .B(II56), .Z(II559) );
  OR2 gate152 ( .A(II482), .B(II590), .Z(II917_2) );
  OR2 gate151 ( .A(II506), .B(II209), .Z(II1099_1) );
  NOR2 gate150 ( .A(II946_1), .B(II946_2), .Z(II209) );
  OR2 gate149 ( .A(cnt13), .B(II506), .Z(II1095_1) );
  NAND2 gate148 ( .A(II535), .B(II58), .Z(II506) );
  OR2 gate147 ( .A(II466), .B(II78), .Z(II962_2) );
  OR2 gate146 ( .A(II466), .B(II627), .Z(II909_1) );
  NOR2 gate145 ( .A(pcnt241), .B(II78), .Z(II627) );
  OR2 gate144 ( .A(II494), .B(II570), .Z(II921_1) );
  OR3 gate143 ( .A(II458), .B(II494), .C(st_5), .Z(II917_1) );
  NAND2 gate142 ( .A(st_3), .B(st_1), .Z(II458) );
  OR2 gate141 ( .A(st_4), .B(II478), .Z(II1092_1) );
  OR2 gate140 ( .A(II58), .B(II478), .Z(II903_2) );
  NAND2 gate139 ( .A(II547), .B(II739), .Z(II478) );
  OR2 gate138 ( .A(II474), .B(II666), .Z(II924_2) );
  OR2 gate137 ( .A(cnt284), .B(II642), .Z(II958_2) );
  NAND2 gate136 ( .A(II739), .B(st_2), .Z(II642) );
  OR2 gate135 ( .A(II57), .B(II59), .Z(II958_1) );
  OR2 gate134 ( .A(II57), .B(II58), .Z(II933_1) );
  OR2 gate133 ( .A(st_4), .B(II590), .Z(II1068_1) );
  OR2 gate132 ( .A(II60), .B(II61), .Z(II914_1) );
  OR2 gate131 ( .A(pcnt27), .B(II73), .Z(II985_1) );
  OR2 gate130 ( .A(cnt21), .B(st_0), .Z(II1038_1) );
  AND2 gate129 ( .A(II61), .B(II216), .Z(II1085_2) );
  AND2 gate128 ( .A(II58), .B(II204), .Z(II1055_2) );
  AND2 gate127 ( .A(st_4), .B(II234), .Z(II1113_1) );
  NAND4 gate126 ( .A(II213), .B(II814), .C(II710), .D(II714), .Z(II234) );
  AND2 gate125 ( .A(II475), .B(II232), .Z(II1065_1) );
  NAND3 gate124 ( .A(II962_1), .B(II962_2), .C(II810), .Z(II232) );
  AND2 gate123 ( .A(II61), .B(II230), .Z(II1116_2) );
  NAND2 gate122 ( .A(II958_1), .B(II958_2), .Z(II230) );
  AND2 gate121 ( .A(st_2), .B(II483), .Z(II1081_2) );
  AND3 gate120 ( .A(st_5), .B(cnt509), .C(II567), .Z(II954_1) );
  AND2 gate119 ( .A(II787), .B(II130), .Z(II1085_1) );
  NOR3 gate118 ( .A(II554), .B(st_5), .C(II574), .Z(II787) );
  AND2 gate117 ( .A(II694), .B(II698), .Z(II988_1) );
  NAND2 gate116 ( .A(II563), .B(II59), .Z(II698) );
  NAND2 gate115 ( .A(II795), .B(II57), .Z(II694) );
  AND2 gate114 ( .A(II104), .B(II539), .Z(II1077_1) );
  NOR2 gate113 ( .A(II546), .B(II60), .Z(II539) );
  AND2 gate112 ( .A(II495), .B(II60), .Z(II940_1) );
  AND2 gate111 ( .A(II57), .B(II543), .Z(II1106_2) );
  AND3 gate110 ( .A(II543), .B(II490), .C(II58), .Z(II1081_1) );
  NAND2 gate109 ( .A(cnt284), .B(pcnt17), .Z(II490) );
  NOR2 gate108 ( .A(II742), .B(II590), .Z(II543) );
  AND3 gate107 ( .A(cnt45), .B(II587), .C(II104), .Z(II954_2) );
  NAND2 gate106 ( .A(II933_1), .B(II56), .Z(II104) );
  NOR2 gate105 ( .A(st_1), .B(st_2), .Z(II587) );
  AND2 gate104 ( .A(st_5), .B(II455), .Z(II1102_1) );
  AND2 gate103 ( .A(II455), .B(cnt45), .Z(II950_1) );
  NOR2 gate102 ( .A(II554), .B(II658), .Z(II455) );
  AND2 gate101 ( .A(II463), .B(cnt283), .Z(II950_2) );
  AND2 gate100 ( .A(II95), .B(II603), .Z(II1116_1) );
  NOR2 gate99 ( .A(II61), .B(II56), .Z(II603) );
  NOR2 gate98 ( .A(II587), .B(II591), .Z(II95) );
  AND2 gate97 ( .A(II795), .B(II618), .Z(II1120_1) );
  NAND2 gate96 ( .A(II69), .B(cnt44), .Z(II618) );
  NOR3 gate95 ( .A(st_3), .B(st_2), .C(II578), .Z(II795) );
  AND2 gate94 ( .A(II535), .B(II598), .Z(II1062_1) );
  NAND2 gate93 ( .A(cnt13), .B(II56), .Z(II598) );
  NOR2 gate92 ( .A(II590), .B(st_0), .Z(II535) );
  AND3 gate91 ( .A(II570), .B(st_0), .C(st_2), .Z(II1055_1) );
  NAND2 gate90 ( .A(II458), .B(II56), .Z(II570) );
  AND2 gate89 ( .A(II498), .B(II57), .Z(II967_1) );
  NAND2 gate88 ( .A(II511), .B(II587), .Z(II498) );
  AND2 gate87 ( .A(II551), .B(II663), .Z(II1123_1) );
  NOR2 gate86 ( .A(st_1), .B(II58), .Z(II663) );
  AND2 gate85 ( .A(II60), .B(II551), .Z(II1106_1) );
  AND2 gate84 ( .A(II551), .B(II671), .Z(II1071_1) );
  NOR2 gate83 ( .A(II61), .B(II57), .Z(II551) );
  AND2 gate82 ( .A(st_5), .B(II671), .Z(II1059_1) );
  NOR2 gate81 ( .A(II458), .B(II59), .Z(II671) );
  AND2 gate80 ( .A(II56), .B(II675), .Z(II1102_2) );
  NOR2 gate79 ( .A(II61), .B(st_1), .Z(II675) );
  AND2 gate78 ( .A(II578), .B(st_3), .Z(II943_1) );
  NAND2 gate77 ( .A(II61), .B(st_1), .Z(II578) );
  AND2 gate76 ( .A(II70), .B(cnt284), .Z(II1044_1) );
  AND2 gate75 ( .A(II59), .B(II555), .Z(II1089_1) );
  AND2 gate74 ( .A(cnt591), .B(II59), .Z(II936_1) );
  AND2 gate73 ( .A(john), .B(st_4), .Z(II946_2) );
  AND2 gate72 ( .A(cnt10), .B(st_5), .Z(II946_1) );
  AND2 gate71 ( .A(cnt272), .B(st_2), .Z(II936_2) );
  INV gate70 ( .A(II217), .Z(II216) );
  NOR2 gate69 ( .A(II954_1), .B(II954_2), .Z(II217) );
  INV gate68 ( .A(II205), .Z(II204) );
  NOR2 gate67 ( .A(II563), .B(II940_1), .Z(II205) );
  INV gate66 ( .A(II346), .Z(II347) );
  NAND2 gate65 ( .A(II985_1), .B(II463), .Z(II346) );
  INV gate64 ( .A(II779), .Z(II778) );
  NOR3 gate63 ( .A(II95), .B(st_4), .C(II638), .Z(II779) );
  INV gate62 ( .A(II606), .Z(II607) );
  NAND2 gate61 ( .A(II95), .B(II57), .Z(II606) );
  INV gate60 ( .A(II487), .Z(II486) );
  NOR2 gate59 ( .A(st_4), .B(II498), .Z(II487) );
  INV gate58 ( .A(II531), .Z(II530) );
  NOR2 gate57 ( .A(II574), .B(II59), .Z(II531) );
  INV gate56 ( .A(II482), .Z(II483) );
  NAND2 gate55 ( .A(II58), .B(II551), .Z(II482) );
  INV gate54 ( .A(II566), .Z(II567) );
  NAND2 gate53 ( .A(II663), .B(st_2), .Z(II566) );
  INV gate52 ( .A(II131), .Z(II130) );
  NOR2 gate51 ( .A(II936_1), .B(II936_2), .Z(II131) );
  INV gate50 ( .A(II463), .Z(II462) );
  NOR2 gate49 ( .A(II458), .B(II594), .Z(II463) );
  INV gate48 ( .A(II466), .Z(II467) );
  NAND2 gate47 ( .A(st_3), .B(II535), .Z(II466) );
  INV gate46 ( .A(II494), .Z(II495) );
  NAND2 gate45 ( .A(II57), .B(II547), .Z(II494) );
  INV gate44 ( .A(II799), .Z(II798) );
  NOR3 gate43 ( .A(II56), .B(II58), .C(II59), .Z(II799) );
  INV gate42 ( .A(II474), .Z(II475) );
  NAND2 gate41 ( .A(II56), .B(II57), .Z(II474) );
  INV gate40 ( .A(II666), .Z(II667) );
  NAND2 gate39 ( .A(II61), .B(st_3), .Z(II666) );
  INV gate38 ( .A(II547), .Z(II546) );
  NOR2 gate37 ( .A(II61), .B(st_2), .Z(II547) );
  INV gate36 ( .A(II595), .Z(II594) );
  NOR2 gate35 ( .A(st_0), .B(st_2), .Z(II595) );
  INV gate34 ( .A(II590), .Z(II591) );
  NAND2 gate33 ( .A(st_1), .B(st_2), .Z(II590) );
  INV gate32 ( .A(II555), .Z(II554) );
  NOR2 gate31 ( .A(st_0), .B(st_1), .Z(II555) );
  INV gate30 ( .A(pcnt17), .Z(II70) );
  INV gate29 ( .A(pcnt12), .Z(II69) );
  INV gate28 ( .A(st_5), .Z(II56) );
  INV gate27 ( .A(st_4), .Z(II57) );
  INV gate26 ( .A(st_3), .Z(II58) );
  INV gate25 ( .A(st_2), .Z(II59) );
  INV gate24 ( .A(st_1), .Z(II60) );
  INV gate23 ( .A(st_0), .Z(II61) );
  INV gate22 ( .A(cnt567), .Z(II73) );
  INV gate21 ( .A(cnt511), .Z(II78) );
  INV gate20 ( .A(cnt261), .Z(II67) );
  INV gate19 ( .A(cnt44), .Z(II68) );
  NAND3 gate17 ( .A(II778), .B(II782), .C(II887), .Z(II7) );
  NAND3 gate15 ( .A(II909_1), .B(II899), .C(II895), .Z(II6) );
  NAND4 gate13 ( .A(II282), .B(II889), .C(II827), .D(II298), .Z(II5) );
  NAND4 gate11 ( .A(II278), .B(II274), .C(II270), .D(II266), .Z(II4) );
  NAND3 gate9 ( .A(II903_1), .B(II903_2), .C(II823), .Z(II3) );
  NAND2 gate7 ( .A(II900_1), .B(II821), .Z(II2) );
  NAND3 gate6 ( .A(II924_1), .B(II924_2), .C(II881), .Z(csync) );
  NAND2 gate5 ( .A(II928_1), .B(II841), .Z(cblank) );
  NAND3 gate4 ( .A(II914_1), .B(II855), .C(II867), .Z(vsync) );
  NAND4 gate3 ( .A(II486), .B(II877), .C(II546), .D(II390), .Z(cclr) );
  NAND2 gate2 ( .A(II921_1), .B(II837), .Z(pc) );
  NAND2 gate1 ( .A(II917_1), .B(II917_2), .Z(pclr) );
  NOR2 gate0 ( .A(II555), .B(II798), .Z(csm) );
endmodule
