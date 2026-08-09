module b02 (CLK, LINEA, U_REG ) ;

input   CLK, LINEA ;

output   U_REG ;

DFF gate0 (.Q(U_REG), .D(U31), .CP(CLK) );
DFF gate1 (.Q(STATO_REG_2_), .D(U33), .CP(CLK) );
DFF gate2 (.Q(STATO_REG_1_), .D(U38), .CP(CLK) );
DFF gate3 (.Q(STATO_REG_0_), .D(U32), .CP(CLK) );

AND3 gate4 (.A(U35), .B(U37), .C(STATO_REG_2_), .Z(U31));
NAND2 gate5 (.A(U48), .B(U47), .Z(U32));
NAND2 gate6 (.A(U42), .B(U41), .Z(U33));
INV gate7 (.A(STATO_REG_2_), .Z(U34));
INV gate8 (.A(STATO_REG_0_), .Z(U35));
INV gate9 (.A(LINEA), .Z(U36));
INV gate10 (.A(STATO_REG_1_), .Z(U37));
NAND2 gate11 (.A(U52), .B(U51), .Z(U38));
NAND3 gate12 (.A(U50), .B(U49), .C(U35), .Z(U39));
OR2 gate13 (.A(STATO_REG_2_), .B(LINEA), .Z(U40));
NAND2 gate14 (.A(STATO_REG_0_), .B(U40), .Z(U41));
NAND2 gate15 (.A(STATO_REG_1_), .B(U39), .Z(U42));
OR2 gate16 (.A(LINEA), .B(STATO_REG_1_), .Z(U43));
NAND2 gate17 (.A(U34), .B(U43), .Z(U44));
NAND2 gate18 (.A(LINEA), .B(U34), .Z(U45));
NAND2 gate19 (.A(STATO_REG_0_), .B(U45), .Z(U46));
OR3 gate20 (.A(LINEA), .B(STATO_REG_0_), .C(STATO_REG_2_), .Z(U47));
NAND2 gate21 (.A(U46), .B(U37), .Z(U48));
NAND2 gate22 (.A(LINEA), .B(U34), .Z(U49));
NAND2 gate23 (.A(STATO_REG_2_), .B(U36), .Z(U50));
NAND2 gate24 (.A(STATO_REG_0_), .B(U44), .Z(U51));
NAND3 gate25 (.A(STATO_REG_1_), .B(U34), .C(U35), .Z(U52));

endmodule
