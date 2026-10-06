== function <main> (params=0, registers=2) ==
0000  L3   GETGLOBAL R0 G0  ; afise
0001  L3   LOADK     R1 K0  ; 42
0002  L3   CALL      R0 1
0003  L4   GETGLOBAL R0 G0  ; afise
0004  L4   LOADK     R1 K1  ; 3.14
0005  L4   CALL      R0 1
0006  L5   GETGLOBAL R0 G0  ; afise
0007  L5   LOADK     R1 K2  ; "Lebon"
0008  L5   CALL      R0 1
0009  L6   GETGLOBAL R0 G0  ; afise
0010  L6   LOADK     R1 K3  ; ""
0011  L6   CALL      R0 1
0012  L7   GETGLOBAL R0 G0  ; afise
0013  L7   LOADBOOL  R1 1
0014  L7   CALL      R0 1
0015  L8   GETGLOBAL R0 G0  ; afise
0016  L8   LOADBOOL  R1 0
0017  L8   CALL      R0 1
0018  L10  GETGLOBAL R0 G0  ; afise
0019  L10  LOADK     R1 K0  ; 42
0020  L10  CALL      R0 1
0021  L11  GETGLOBAL R0 G0  ; afise
0022  L11  LOADK     R1 K2  ; "Lebon"
0023  L11  CALL      R0 1
0024  L11  RETURN    R0 0
