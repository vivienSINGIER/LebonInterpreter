== function <main> (params=0, registers=4) ==
0000  L3   GETGLOBAL R0 K0  ; "afise"
0001  L3   GETGLOBAL R1 K0  ; "afise"
0002  L3   LOADK     R2 K1  ; 7
0003  L3   CALL      R1 1
0004  L3   CALL      R0 1
0005  L4   GETGLOBAL R0 K0  ; "afise"
0006  L4   GETGLOBAL R1 K0  ; "afise"
0007  L4   LOADK     R2 K2  ; 1
0008  L4   LOADK     R3 K3  ; 2
0009  L4   ADD       R2 R2 R3
0010  L4   CALL      R1 1
0011  L4   LOADK     R2 K4  ; 3
0012  L4   MUL       R1 R1 R2
0013  L4   CALL      R0 1
0014  L5   LOADK     R0 K2  ; 1
0015  L5   LOADK     R1 K3  ; 2
0016  L5   ADD       R0 R0 R1
0017  L6   LOADK     R0 K5  ; "seul"
0018  L7   LOADK     R0 K6  ; 5
0019  L7   NEG       R0 R0
0020  L7   LOADK     R1 K3  ; 2
0021  L7   MUL       R0 R0 R1
0022  L7   RETURN    R0 0
