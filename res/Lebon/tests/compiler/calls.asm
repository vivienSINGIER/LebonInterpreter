== function <main> (params=0, registers=3) ==
0000  L3   GETGLOBAL R0 K0  ; "afise"
0001  L3   LOADK     R1 K1  ; 7
0002  L3   CALL      R0 1
0003  L4   GETGLOBAL R0 K0  ; "afise"
0004  L4   LOADK     R1 K2  ; 1
0005  L4   LOADK     R2 K3  ; 2
0006  L4   ADD       R1 R1 R2
0007  L4   LOADK     R2 K4  ; 3
0008  L4   MUL       R1 R1 R2
0009  L4   CALL      R0 1
0010  L5   LOADK     R0 K2  ; 1
0011  L5   LOADK     R1 K3  ; 2
0012  L5   ADD       R0 R0 R1
0013  L6   LOADK     R0 K5  ; "seul"
0014  L7   LOADK     R0 K6  ; 5
0015  L7   NEG       R0 R0
0016  L7   LOADK     R1 K3  ; 2
0017  L7   MUL       R0 R0 R1
0018  L7   RETURN    R0 0
