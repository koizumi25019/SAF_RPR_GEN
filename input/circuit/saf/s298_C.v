
module s298_C ( G98, G92, G86, G67, G66, G56, G44, G39, G34, G30, G29, G23, 
        G22, G21, G20, G2, G19, G18, G17, G16, G15, G14, G133, G132, G13, G125, 
        G12, G119, G118, G117, G113, G11, G107, G102, G10, G1, G0 );
  input G23, G22, G21, G20, G2, G19, G18, G17, G16, G15, G14, G13, G12, G11,
         G10, G1, G0;
  output G98, G92, G86, G67, G66, G56, G44, G39, G34, G30, G29, G133, G132,
         G125, G119, G118, G117, G113, G107, G102;
  wire   G100, G101, G103, G104, G105, G106, G108, G109, G110, G111, G112,
         G114, G115, G116, G120, G121, G122, G123, G124, G126, G127, G128,
         G129, G130, G131, G24, G25, G26, G27, G28, G31, G32, G33, G35, G36,
         G37, G38, G40, G41, G42, G43, G45, G46, G47, G48, G49, G50, G51, G52,
         G53, G54, G55, G57, G58, G59, G60, G61, G62, G63, G64, G65, G68, G69,
         G70, G71, G72, G73, G74, G75, G76, G77, G78, G79, G80, G81, G82, G83,
         G84, G85, G87, G88, G89, G90, G91, G93, G94, G95, G96, G97, G99,
         II155, II158, II210, II213, II221, II229, II232, II235, II238;

  NOR2 gate132 ( .A(G26), .B(G27), .Z(G53) );
  NAND4 gate131 ( .A(G79), .B(G80), .C(G81), .D(G108), .Z(G116) );
  NAND2 gate130 ( .A(G77), .B(G78), .Z(G106) );
  NAND4 gate129 ( .A(G68), .B(G69), .C(G70), .D(G108), .Z(G101) );
  NAND4 gate128 ( .A(G83), .B(G84), .C(G85), .D(G108), .Z(G97) );
  NAND3 gate127 ( .A(G24), .B(G25), .C(G28), .Z(G43) );
  OR3 gate126 ( .A(G91), .B(G96), .C(G17), .Z(G85) );
  OR3 gate125 ( .A(G82), .B(G91), .C(G14), .Z(G84) );
  OR4 gate124 ( .A(G11), .B(G12), .C(G13), .D(G96), .Z(G83) );
  OR2 gate123 ( .A(G12), .B(G13), .Z(G81) );
  OR2 gate122 ( .A(G11), .B(G14), .Z(G80) );
  OR2 gate121 ( .A(G103), .B(G14), .Z(G79) );
  OR2 gate120 ( .A(G108), .B(G76), .Z(G78) );
  OR4 gate119 ( .A(G112), .B(G103), .C(G96), .D(G19), .Z(G77) );
  OR2 gate118 ( .A(G103), .B(G20), .Z(G73) );
  OR2 gate117 ( .A(G91), .B(G20), .Z(G72) );
  OR3 gate116 ( .A(G82), .B(G12), .C(G13), .Z(G71) );
  OR2 gate115 ( .A(G103), .B(G14), .Z(G70) );
  OR2 gate114 ( .A(G103), .B(G18), .Z(G69) );
  OR4 gate113 ( .A(G11), .B(G12), .C(G13), .D(G96), .Z(G68) );
  OR3 gate112 ( .A(G38), .B(G11), .C(G12), .Z(G25) );
  OR4 gate111 ( .A(G38), .B(G46), .C(G45), .D(G40), .Z(G24) );
  AND2 gate110 ( .A(G131), .B(G23), .Z(G129) );
  AND2 gate109 ( .A(G126), .B(G127), .Z(G128) );
  AND2 gate108 ( .A(G124), .B(G22), .Z(G123) );
  AND2 gate107 ( .A(G120), .B(G121), .Z(G122) );
  AND2 gate106 ( .A(G114), .B(G14), .Z(G115) );
  AND2 gate105 ( .A(G10), .B(G112), .Z(G111) );
  AND2 gate104 ( .A(G108), .B(G109), .Z(G110) );
  NAND4 gate103 ( .A(G71), .B(G72), .C(G73), .D(G14), .Z(G109) );
  AND3 gate102 ( .A(G103), .B(G108), .C(G104), .Z(G105) );
  NOR2 gate101 ( .A(G74), .B(G75), .Z(G104) );
  AND3 gate100 ( .A(G99), .B(G14), .C(G12), .Z(G100) );
  AND2 gate99 ( .A(G96), .B(G13), .Z(G95) );
  AND2 gate98 ( .A(G93), .B(G13), .Z(G94) );
  AND2 gate97 ( .A(G91), .B(G103), .Z(G90) );
  AND2 gate96 ( .A(G103), .B(G96), .Z(G89) );
  AND2 gate95 ( .A(G14), .B(G87), .Z(G88) );
  AND3 gate94 ( .A(G82), .B(G91), .C(G14), .Z(G75) );
  AND3 gate93 ( .A(G12), .B(G14), .C(G19), .Z(G74) );
  AND2 gate92 ( .A(G64), .B(G65), .Z(G63) );
  AND4 gate91 ( .A(G59), .B(G11), .C(G60), .D(G61), .Z(G62) );
  AND2 gate90 ( .A(G64), .B(G65), .Z(G58) );
  NAND4 gate89 ( .A(G59), .B(G54), .C(G22), .D(G61), .Z(G65) );
  AND4 gate88 ( .A(G59), .B(G11), .C(G60), .D(G61), .Z(G57) );
  NOR2 gate87 ( .A(G14), .B(G55), .Z(G61) );
  AND3 gate86 ( .A(G50), .B(G51), .C(G52), .Z(G49) );
  NAND4 gate85 ( .A(G13), .B(G45), .C(G46), .D(G10), .Z(G52) );
  AND4 gate84 ( .A(G45), .B(G46), .C(G10), .D(G47), .Z(G48) );
  NOR2 gate83 ( .A(G50), .B(G40), .Z(G47) );
  AND2 gate82 ( .A(G40), .B(G41), .Z(G42) );
  NAND3 gate81 ( .A(G12), .B(G11), .C(G10), .Z(G41) );
  AND2 gate80 ( .A(G46), .B(G45), .Z(G37) );
  AND2 gate79 ( .A(G38), .B(G45), .Z(G36) );
  AND3 gate78 ( .A(G10), .B(G11), .C(G12), .Z(G35) );
  AND2 gate77 ( .A(G38), .B(G46), .Z(G33) );
  AND2 gate76 ( .A(G10), .B(G11), .Z(G32) );
  AND3 gate75 ( .A(G10), .B(G45), .C(G13), .Z(G31) );
  AND2 gate74 ( .A(G51), .B(G28), .Z(G27) );
  AND2 gate73 ( .A(G28), .B(G50), .Z(G26) );
  INV gate72 ( .A(G21), .Z(II238) );
  INV gate71 ( .A(G20), .Z(II235) );
  INV gate70 ( .A(G1), .Z(II232) );
  INV gate69 ( .A(G0), .Z(II229) );
  INV gate68 ( .A(G23), .Z(G127) );
  INV gate67 ( .A(G131), .Z(G126) );
  INV gate66 ( .A(II232), .Z(G131) );
  INV gate65 ( .A(G2), .Z(II221) );
  INV gate64 ( .A(G22), .Z(G121) );
  INV gate63 ( .A(G124), .Z(G120) );
  INV gate62 ( .A(II221), .Z(G124) );
  INV gate61 ( .A(G19), .Z(II213) );
  INV gate60 ( .A(G18), .Z(II210) );
  INV gate59 ( .A(G21), .Z(G114) );
  INV gate58 ( .A(G112), .Z(G108) );
  NOR2 gate57 ( .A(G62), .B(G63), .Z(G112) );
  INV gate56 ( .A(G13), .Z(G103) );
  INV gate55 ( .A(G18), .Z(G99) );
  INV gate54 ( .A(G14), .Z(G96) );
  INV gate53 ( .A(G17), .Z(G93) );
  INV gate52 ( .A(G12), .Z(G91) );
  INV gate51 ( .A(G16), .Z(G87) );
  INV gate50 ( .A(G11), .Z(G82) );
  INV gate49 ( .A(G10), .Z(G76) );
  INV gate48 ( .A(G17), .Z(II158) );
  INV gate47 ( .A(G16), .Z(II155) );
  INV gate46 ( .A(G15), .Z(G64) );
  INV gate45 ( .A(G22), .Z(G60) );
  INV gate44 ( .A(G12), .Z(G59) );
  INV gate43 ( .A(G13), .Z(G55) );
  INV gate42 ( .A(G11), .Z(G54) );
  INV gate41 ( .A(G23), .Z(G51) );
  INV gate40 ( .A(G14), .Z(G50) );
  INV gate39 ( .A(G11), .Z(G46) );
  INV gate38 ( .A(G12), .Z(G45) );
  INV gate37 ( .A(G13), .Z(G40) );
  INV gate36 ( .A(G10), .Z(G38) );
  INV gate35 ( .A(G130), .Z(G28) );
  INV gate34 ( .A(II229), .Z(G130) );
  NOR3 gate32 ( .A(G128), .B(G129), .C(G130), .Z(G125) );
  NOR3 gate30 ( .A(G122), .B(G123), .C(G130), .Z(G119) );
  NOR2 gate28 ( .A(G115), .B(G116), .Z(G113) );
  NOR2 gate26 ( .A(G110), .B(G111), .Z(G107) );
  NOR2 gate24 ( .A(G105), .B(G106), .Z(G102) );
  NOR2 gate22 ( .A(G100), .B(G101), .Z(G98) );
  NOR3 gate20 ( .A(G94), .B(G95), .C(G97), .Z(G92) );
  NOR4 gate18 ( .A(G88), .B(G89), .C(G90), .D(G112), .Z(G86) );
  NOR3 gate16 ( .A(G57), .B(G58), .C(G130), .Z(G56) );
  NOR3 gate14 ( .A(G48), .B(G49), .C(G53), .Z(G44) );
  NOR2 gate12 ( .A(G42), .B(G43), .Z(G39) );
  NOR4 gate10 ( .A(G35), .B(G36), .C(G37), .D(G130), .Z(G34) );
  NOR4 gate8 ( .A(G31), .B(G32), .C(G33), .D(G130), .Z(G30) );
  NOR2 gate6 ( .A(G10), .B(G130), .Z(G29) );
  INV gate5 ( .A(II158), .Z(G67) );
  INV gate4 ( .A(II238), .Z(G133) );
  INV gate3 ( .A(II213), .Z(G118) );
  INV gate2 ( .A(II155), .Z(G66) );
  INV gate1 ( .A(II235), .Z(G132) );
  INV gate0 ( .A(II210), .Z(G117) );
endmodule
