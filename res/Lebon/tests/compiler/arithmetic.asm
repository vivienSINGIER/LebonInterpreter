== function <main> (params=0, registers=3) ==
0000  L3   GETGLOBAL R0 G0  ; afise
0001  L3   LOADK     R1 K2  ; 2
0002  L3   MULK      R1 R1 K1  ; 3
0003  L3   ADDK      R1 R1 K0  ; 1
0004  L3   CALL      R0 1
0005  L4   GETGLOBAL R0 G0  ; afise
0006  L4   LOADK     R1 K0  ; 1
0007  L4   ADDK      R1 R1 K2  ; 2
0008  L4   MULK      R1 R1 K1  ; 3
0009  L4   CALL      R0 1
0010  L5   GETGLOBAL R0 G0  ; afise
0011  L5   LOADK     R1 K4  ; 10
0012  L5   SUBK      R1 R1 K3  ; 4
0013  L5   SUBK      R1 R1 K0  ; 1
0014  L5   CALL      R0 1
0015  L6   GETGLOBAL R0 G0  ; afise
0016  L6   LOADK     R1 K6  ; 100
0017  L6   DIVK      R1 R1 K5  ; 5
0018  L6   DIVK      R1 R1 K2  ; 2
0019  L6   CALL      R0 1
0020  L7   GETGLOBAL R0 G0  ; afise
0021  L7   LOADK     R1 K3  ; 4
0022  L7   NEG       R1 R1
0023  L7   CALL      R0 1
0024  L8   GETGLOBAL R0 G0  ; afise
0025  L8   LOADK     R1 K0  ; 1
0026  L8   ADDK      R1 R1 K2  ; 2
0027  L8   NEG       R1 R1
0028  L8   MULK      R1 R1 K1  ; 3
0029  L8   CALL      R0 1
0030  L9   GETGLOBAL R0 G0  ; afise
0031  L9   LOADK     R1 K0  ; 1
0032  L9   ADDK      R1 R1 K2  ; 2
0033  L9   LOADK     R2 K1  ; 3
0034  L9   SUBK      R2 R2 K5  ; 5
0035  L9   MUL       R1 R1 R2
0036  L9   DIVK      R1 R1 K3  ; 4
0037  L9   CALL      R0 1
0038  L10  GETGLOBAL R0 G0  ; afise
0039  L10  LOADK     R1 K1  ; 3
0040  L10  ADDK      R1 R1 K3  ; 4
0041  L10  ADDK      R1 R1 K2  ; 2
0042  L10  ADDK      R1 R1 K0  ; 1
0043  L10  CALL      R0 1
0044  L11  GETGLOBAL R0 G0  ; afise
0045  L11  LOADK     R1 K9  ; "Bonzour "
0046  L11  CONCATK   R1 R1 K8  ; "Lebon"
0047  L11  CONCATK   R1 R1 K7  ; " !"
0048  L11  CALL      R0 1
0049  L11  RETURN    R0 0
