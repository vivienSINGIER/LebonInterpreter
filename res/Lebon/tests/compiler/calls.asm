== function <main> (params=0, registers=2) ==
0000  L3   GETGLOBAL R0 G0  ; afise
0001  L3   LOADK     R1 K0  ; 7
0002  L3   CALL      R0 1
0003  L4   GETGLOBAL R0 G0  ; afise
0004  L4   LOADK     R1 K3  ; 1
0005  L4   ADDK      R1 R1 K2  ; 2
0006  L4   MULK      R1 R1 K1  ; 3
0007  L4   CALL      R0 1
0008  L5   LOADK     R0 K3  ; 1
0009  L5   ADDK      R0 R0 K2  ; 2
0010  L6   LOADK     R0 K4  ; "seul"
0011  L7   LOADK     R0 K5  ; 5
0012  L7   NEG       R0 R0
0013  L7   MULK      R0 R0 K2  ; 2
0014  L7   RETURN    R0 0
