 title Emulation des /370-Instruktionssatzes auf einem 80386-Prozessor
 subttl XLAT-Tabelle ASCII nach EBCDIC
; 23. 9.88                                       Copyright     Joacim Zschimmer
          include ebcdisym.inc

tbascebc_text     segment 'code'
          public    _tbascebc
_tbascebc label byte
          public    Tab_ASCII_to_EBCDIC
Tab_ASCII_to_EBCDIC label byte

nix equ 41h

;              0      1      2      3      4      5      6      7

t_00      DB   _eNUL, _eSOH, _eSTX, _eETX, _eEOT, _eENQ, _eACK, _eBEL
t_08      DB   _eBS , _eHT , _eLF , _eVT , _eFF , _eCR , _eSO , _eSI
t_10      DB   _eDLE, _eDC1, _eDC2, _eDC3, _eDC4; _eNAK, _eSYN, _eETB
          DB                                       0B5h, _eSYN, _eETB
t_18      DB   _eCAN, _eEM , _eSUB, _eESC, _eFS , _eGS , _eRS , _eUS

;                 0    1    2    3    4    5    6    7
;                 8    9    A    B    C    D    E    F

t_20      DB    40h, 5Ah, 7Fh, 7Bh, 5Bh, 6Ch, 50h, 7Dh  ;  !#"$%&'
t_28      DB    4Dh, 5Dh, 5Ch, 4Eh, 6Bh, 60h, 4Bh, 61h  ; ()*+,-./
t_30      DB   0F0h,0F1h,0F2h,0F3h,0F4h,0F5h,0F6h,0F7h  ; 01234567
t_38      DB   0F8h,0F9h, 7Ah, 5Eh, 4Ch, 7Eh, 6Eh, 6Fh  ; 89:;<=>?
t_40      DB   07Ch,0C1h,0C2h,0C3h,0C4h,0C5h,0C6h,0C7h  ; @ABCDEFG
t_48      DB   0C8h,0C9h,0D1h,0D2h,0D3h,0D4h,0D5h,0D6h  ; HIJKLMNO
t_50      DB   0D7h,0D8h,0D9h,0E2h,0E3h,0E4h,0E5h,0E6h  ; PQRSTUVW
t_58      DB   0E7h,0E8h,0E9h,0BBh,0BCh,0BDh, 6Ah, 6Dh  ; XYZ[\]^_
t_60      DB    4Ah, 81h, 82h, 83h, 84h, 85h, 86h, 87h  ; `abcdefg
t_68      DB    88h, 89h, 91h, 92h, 93h, 94h, 95h, 96h  ; hijklmno
t_70      DB    97h, 98h, 99h,0A2h,0A3h,0A4h,0A5h,0A6h  ; pqrstuvw
t_78      DB   0A7h,0A8h,0A9h,0FBh, 4Fh,0FDh,0FFh,_eDEL ; xyz{|}~±

t_80      DB    68h,0DCh, 51h, 42h, 43h, 44h, 47h, 48h  ; ÄÅÇÉÑÖÜá
t_88      DB    52h, 53h, 54h, 57h, 56h, 58h, 63h, 67h  ; àâäãåçéè
t_90      DB    71h, 9Ch, 9Eh,0CBh,0CCh,0CDh,0DBh,0C0h  ; êëíìîïñó
t_98      DB   0DFh,0ECh,0FCh,0B0h,0B1h,0B2h, nix, nix  ; òôöõúùûü
t_A0      DB    45h, 55h,0CEh,0DEh, 49h, 69h, 9Ah, 9Bh  ; †°¢£§•¶ß
t_A8      DB   0ABh, nix,0BAh,0B8h,0B7h, nix, 8Ah, 8Bh  ; ®©™´¨≠ÆØ
t_B0      DB    nix, nix, nix, nix, nix, nix, nix, nix  ; ∞±≤≥¥µ∂∑
t_B8      DB    nix, nix, nix, nix, nix, nix, nix, nix  ; ∏π∫ªºΩæø
t_C0      DB    nix, nix, nix, nix, nix, nix, nix, nix  ; ¿¡¬√ƒ≈∆«
t_C8      DB    nix, nix, nix, nix, nix, nix, nix, nix  ; »… ÀÃÕŒœ
t_D0      DB    nix, nix, nix, nix, nix, nix, nix, nix  ; –—“”‘’÷◊
t_D8      DB    nix, nix, nix, nix, nix, nix, nix, nix  ; ÿŸ⁄€‹›ﬁﬂ
t_E0      DB    nix, 59h, nix, nix, nix, nix,0A0h, nix  ; ‡·‚„‰ÂÊÁ
t_E8      DB    nix, nix, nix, nix, nix, 70h, nix, nix  ; ËÈÍÎÏÌÓÔ
t_F0      DB    nix, nix, nix, nix, nix, nix,0E1h, nix  ; ÒÚÛÙıˆ˜
t_F8      DB    90h,0B3h, nix, nix, nix,0EAh, nix, nix  ; ¯˘˙˚¸˝˛

tbascebc_text     ends
          END
