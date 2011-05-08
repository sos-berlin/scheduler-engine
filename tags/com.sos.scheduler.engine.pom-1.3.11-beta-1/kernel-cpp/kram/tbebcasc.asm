 title Emulation des /370-Instruktionssatzes auf einem 80386-Prozessor
 subttl XLAT-Tabelle EBCDIC nach ASCII
;  7. 9.88                                       Copyright     Joacim Zschimmer
        include asciisym.inc

TBEBCASC_TEXT   segment 'CODE'

        public  tbebcasc
        public  _tbebcasc

tbebcasc label  byte
_tbebcasc label byte

x equ '®'

; ISO 8859-1  --  Extended BS2000 Code EBCDIC.DF.04:

;          0    1    2    3  4   5    6    7    8   9 A   B    C    D    E    F

t_00 DB _NUL,_SOH,_STX,_ETX, x,_HT,   x,_DEL,   x,  x,x,_VT,_FF , _CR, _SO,_SI
t_10 DB _DLE,_DC1,_DC2,_DC3, x,  x,_BS ,   x,_CAN,_EM,x,  x,_IS4,_IS3,_IS2,_IS1
t_20 DB    x,   x,   x,   x, x,_LF,_ETB,_ESC,   x,  x,x,  x,   x,_ENQ,_ACK,_BEL
t_30 DB    x,   x,_SYN,   x, x,  x,   x,_EOT,   x,  x,x,  x,_DC4,_NAK,   x,_SUB

;           0   1   2   3   4   5   6   7   8   9    A   B   C   D   E   F

t_40 DB   ' ',' ','É','Ñ','Ö','†','a','Ü','á','§', '`','.','<','(','+','|'
t_50 DB   '&','Ç','à','â','ä','°','å','ã','ç','·', '!','$','*',')',';', x
t_60 DB   '-','/','A','é','A','A','A','è','C','•', '^',',','%','_','>','?'
t_70 DB   'Ì','ê','E','E','E','I','I','I','I','"', ':','#','@','''','=','"'
                                     
t_80 DB   'Ì','a','b','c','d','e','f','g','h','i', 'Æ','Ø', x ,'y', x ,'Ò'
t_90 DB   '¯','j','k','l','m','n','o','p','q','r', '¶','ß','ë',',','í', x
t_A0 DB   'Ê', x ,'s','t','u','v','w','x','y','z',  x ,'®','D','Y', x , x 
t_B0 DB   'õ','ú','ù','˘', x ,'', x ,'¨','´', x , '™','[','\',']','''','x'

t_C0 DB   'ó','A','B','C','D','E','F','G','H','I',  x ,'ì','î','ï','¢', x 
t_D0 DB   '|','J','K','L','M','N','O','P','Q','R',  x ,'ñ','Å','U','£','ò'
t_E0 DB   'U','ˆ','S','T','U','V','W','X','Y','Z', '˝','O','ô','O','O','O'
t_F0 DB   '0','1','2','3','4','5','6','7','8','9',  x ,'{','ö','}','U','~'

if 0;  Standard-EBCDIC
;          0    1    2    3  4   5    6    7    8   9 A   B    C    D    E    F

t_00 DB _NUL,_SOH,_STX,_ETX, x,_HT,   x,_DEL,   x,  x,x,_VT,_FF , _CR, _SO,_SI
t_10 DB _DLE,_DC1,_DC2,_DC3, x,  x,_BS ,   x,_CAN,_EM,x,  x,_IS4,_IS3,_IS2,_IS1
t_20 DB    x,   x,   x,   x, x,_LF,_ETB,_ESC,   x,  x,x,  x,   x,_ENQ,_ACK,_BEL
t_30 DB    x,   x,_SYN,   x, x,  x,   x,_EOT,   x,  x,x,  x,_DC4,_NAK,   x,_SUB

;           0   1   2   3   4   5   6   7   8   9    A   B   C   D   E   F

t_40 DB   ' ',  x,  x,  x,  x,  x,  x,  x,  x,  x, '`','.','<','(','+','|'
t_50 DB   '&',  x,  x,  x,  x,  x,  x,  x,  x,  x, '!','$','*',')',';',x
t_60 DB   '-','/',  x,  x,  x,  x,  x,'·',  x,  x, '^',',','%','_','>','?'
t_70 DB     x,  x,  x,  x,  x,  x,  x,  x,  x,'`', ':','#','@','''','=','"'

t_80 DB     x,'a','b','c','d','e','f','g','h','i',   x,'é','ô','ö',  x,  x
t_90 DB     x,'j','k','l','m','n','o','p','q','r',   x,  x,  x,  x,  x,  x
t_A0 DB     x,  x,'s','t','u','v','w','x','y','z',   x,'Ñ','î','Å',  x,  x
t_B0 DB     x,  x,  x,  x,  x,  x,  x,  x,  x,  x,   x,'[','\',']',  x,  x

t_C0 DB     x,'A','B','C','D','E','F','G','H','I',   x,  x,  x,  x,  x,  x
t_D0 DB     x,'J','K','L','M','N','O','P','Q','R',   x,  x,  x,  x,  x,  x
t_E0 DB     x,  x,'S','T','U','V','W','X','Y','Z',   x,  x,  x,  x,  x,  x
t_F0 DB   '0','1','2','3','4','5','6','7','8','9',   x,'{',  x,'}',  x,'~'
endif

TBEBCASC_TEXT   ends
        END
