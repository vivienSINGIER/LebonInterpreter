== function <main> (params=0, registers=5) ==
0000  L3   CLOSURE   R0 P0  ; addition
0001  L3   SETGLOBAL R0 G1  ; addition
0002  L8   CLOSURE   R0 P1  ; prézant
0003  L8   SETGLOBAL R0 G2  ; prézant
0004  L14  CLOSURE   R0 P2  ; faktoryel
0005  L14  SETGLOBAL R0 G3  ; faktoryel
0006  L19  CLOSURE   R0 P3  ; compteur
0007  L19  SETGLOBAL R0 G4  ; compteur
0008  L30  GETGLOBAL R0 G1  ; addition
0009  L30  LOADK     R1 K0  ; 2
0010  L30  LOADK     R2 K1  ; 3
0011  L30  CALL      R0 2
0012  L30  SETGLOBAL R0 G5  ; somme
0013  L31  GETGLOBAL R0 G0  ; afise
0014  L31  GETGLOBAL R1 G5  ; somme
0015  L31  CALL      R0 1
0016  L32  GETGLOBAL R0 G0  ; afise
0017  L32  GETGLOBAL R1 G1  ; addition
0018  L32  LOADK     R2 K2  ; 1
0019  L32  GETGLOBAL R3 G3  ; faktoryel
0020  L32  LOADK     R4 K1  ; 3
0021  L32  CALL      R3 1
0022  L32  CALL      R1 2
0023  L32  CALL      R0 1
0024  L33  GETGLOBAL R0 G0  ; afise
0025  L33  GETGLOBAL R1 G4  ; compteur
0026  L33  LOADK     R2 K3  ; 10
0027  L33  CALL      R1 1
0028  L33  CALL      R0 1
0029  L34  GETGLOBAL R0 G2  ; prézant
0030  L34  CALL      R0 0
0031  L34  RETURN    R0 0

== function addition (params=2, registers=4) ==
0000  L5   MOVE      R2 R0
0001  L5   MOVE      R3 R1
0002  L5   ADD       R2 R2 R3
0003  L5   RETURN    R2 1
0004  L5   RETURN    R0 0

== function prézant (params=0, registers=2) ==
0000  L10  GETGLOBAL R0 G0  ; afise
0001  L10  LOADK     R1 K0  ; "Lebon"
0002  L10  CALL      R0 1
0003  L11  RETURN    R0 0
0004  L11  RETURN    R0 0

== function faktoryel (params=1, registers=5) ==
0000  L16  MOVE      R1 R0
0001  L16  GETGLOBAL R2 G3  ; faktoryel
0002  L16  MOVE      R3 R0
0003  L16  LOADK     R4 K0  ; 1
0004  L16  SUB       R3 R3 R4
0005  L16  CALL      R2 1
0006  L16  MUL       R1 R1 R2
0007  L16  RETURN    R1 1
0008  L16  RETURN    R0 0

== function compteur (params=1, registers=5) ==
0000  L21  MOVE      R1 R0
0001  L22  CLOSURE   R2 P0  ; ajoute
0002  L27  MOVE      R3 R2
0003  L27  LOADK     R4 K0  ; 1
0004  L27  CALL      R3 1
0005  L27  RETURN    R3 1
0006  L27  RETURN    R0 0

== function ajoute (params=1, registers=3) ==
0000  L24  GETUPVAL  R1 U0
0001  L24  MOVE      R2 R0
0002  L24  ADD       R1 R1 R2
0003  L24  SETUPVAL  R1 U0
0004  L25  GETUPVAL  R1 U0
0005  L25  RETURN    R1 1
0006  L25  RETURN    R0 0
