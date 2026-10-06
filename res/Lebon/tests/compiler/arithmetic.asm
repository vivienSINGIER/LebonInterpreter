== function <main> (params=0, registers=5) ==
0000  L3   GETGLOBAL R0 K0  ; "afise"
0001  L3   LOADK     R1 K1  ; 1
0002  L3   LOADK     R2 K2  ; 2
0003  L3   LOADK     R3 K3  ; 3
0004  L3   MUL       R2 R2 R3
0005  L3   ADD       R1 R1 R2
0006  L3   CALL      R0 1
0007  L4   GETGLOBAL R0 K0  ; "afise"
0008  L4   LOADK     R1 K1  ; 1
0009  L4   LOADK     R2 K2  ; 2
0010  L4   ADD       R1 R1 R2
0011  L4   LOADK     R2 K3  ; 3
0012  L4   MUL       R1 R1 R2
0013  L4   CALL      R0 1
0014  L5   GETGLOBAL R0 K0  ; "afise"
0015  L5   LOADK     R1 K4  ; 10
0016  L5   LOADK     R2 K5  ; 4
0017  L5   SUB       R1 R1 R2
0018  L5   LOADK     R2 K1  ; 1
0019  L5   SUB       R1 R1 R2
0020  L5   CALL      R0 1
0021  L6   GETGLOBAL R0 K0  ; "afise"
0022  L6   LOADK     R1 K6  ; 100
0023  L6   LOADK     R2 K7  ; 5
0024  L6   DIV       R1 R1 R2
0025  L6   LOADK     R2 K2  ; 2
0026  L6   DIV       R1 R1 R2
0027  L6   CALL      R0 1
0028  L7   GETGLOBAL R0 K0  ; "afise"
0029  L7   LOADK     R1 K5  ; 4
0030  L7   NEG       R1 R1
0031  L7   CALL      R0 1
0032  L8   GETGLOBAL R0 K0  ; "afise"
0033  L8   LOADK     R1 K1  ; 1
0034  L8   LOADK     R2 K2  ; 2
0035  L8   ADD       R1 R1 R2
0036  L8   NEG       R1 R1
0037  L8   LOADK     R2 K3  ; 3
0038  L8   MUL       R1 R1 R2
0039  L8   CALL      R0 1
0040  L9   GETGLOBAL R0 K0  ; "afise"
0041  L9   LOADK     R1 K1  ; 1
0042  L9   LOADK     R2 K2  ; 2
0043  L9   ADD       R1 R1 R2
0044  L9   LOADK     R2 K3  ; 3
0045  L9   LOADK     R3 K7  ; 5
0046  L9   SUB       R2 R2 R3
0047  L9   MUL       R1 R1 R2
0048  L9   LOADK     R2 K5  ; 4
0049  L9   DIV       R1 R1 R2
0050  L9   CALL      R0 1
0051  L10  GETGLOBAL R0 K0  ; "afise"
0052  L10  LOADK     R1 K1  ; 1
0053  L10  LOADK     R2 K2  ; 2
0054  L10  LOADK     R3 K3  ; 3
0055  L10  LOADK     R4 K5  ; 4
0056  L10  ADD       R3 R3 R4
0057  L10  ADD       R2 R2 R3
0058  L10  ADD       R1 R1 R2
0059  L10  CALL      R0 1
0060  L11  GETGLOBAL R0 K0  ; "afise"
0061  L11  LOADK     R1 K8  ; "Bonzour "
0062  L11  LOADK     R2 K9  ; "Lebon"
0063  L11  ADD       R1 R1 R2
0064  L11  LOADK     R2 K10  ; " !"
0065  L11  ADD       R1 R1 R2
0066  L11  CALL      R0 1
0067  L11  RETURN    R0 0
