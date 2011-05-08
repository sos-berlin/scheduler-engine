; Include fÅr "strukturierte" Programmierung in Assembler.
; 16. 9.88						       Joacim Zschimmer
	page    65, 132
	.xlist
	.xcref
	.386

; Dieser Include mu· in Pass 1 und in Pass 2 aufgerufen werden.

	if1
	    include jzincl1.inc
	endif

;;	    ifndef model
;;	       model equ <small>
;;	    endif
;;	    @source , .model, %model

_CPU_Taktfrequenz = 66*3000000          ; 66 MHz

screen_segment equ 0B800h               ; Bildschirmspeicher

true	  EQU	  -1
false	  EQU	  0

_86_CF	  EQU			    00000001b
_86_PF	  EQU			    00000100b
_86_AF	  EQU			    00010000b
_86_ZF	  EQU			    01000000b
_86_SF	  EQU			    10000000b

_86_TF	  EQU		  00000001b shl 8
_86_IF	  EQU		  00000010b shl 8
_86_DF	  EQU		  00000100b shl 8
_86_OF	  EQU		  00001000b shl 8
_286_IOPL EQU		  00110000b shl 8
_286_IOPL_Factor EQU	  00001000b shl 8
_286_NT   EQU		  01000000b shl 8

_86_user_flags EQU (_86_OF+_86_DF+_86_SF+_86_ZF+_86_AF+_86_PF+_86_CF)
 
_386_RT   EQU	00000001b shl 16
_386_VM   EQU	00000010b shl 16

Prefix_REP      EQU  0F3h
Prefix_REPE     EQU  0F3h
Prefix_REPZ     EQU  0F3h
Prefix_REPNE    EQU  0F2h
Prefix_REPNZ    EQU  0F2h
Prefix_LOCK     EQU  0F0h
Prefix_CS       EQU  02Eh
Prefix_SS       EQU  036h
Prefix_DS       EQU  03Eh
Prefix_ES       EQU  026h
Prefix_FS       EQU  064h
Prefix_GS       EQU  065h
Prefix_Operand_Size EQU  066h
Prefix_Address_Size EQU  067h


@@j_eq          equ   je
@@j_ne          equ   jne
@@j_lt          equ   jl
@@j_gt          equ   jg
@@j_le          equ   jle
@@j_ge          equ   jge
@@j_of          equ   jo
@@j_ze          equ   jz
@@j_nz          equ   jnz
@@j_gz          equ   jg
@@j_lz          equ   jl
@@j_above       equ   ja
@@j_not_above   equ   jna
@@j_below       equ   jb
@@j_not_below   equ   jnb
@@j_ae          equ   jae
@@j_above_or_equal equ   jae
@@j_be          equ   jbe
@@j_below_or_equal equ   jbe
@@j_carry       equ   jc
@@j_no_carry    equ   jnc
@@j_overflow    equ   jo
@@j_no_overflow equ   jno
@@j_true        equ   jnz
@@j_false       equ   jz

@@jn_eq         equ   jne
@@jn_ne         equ   je
@@jn_lt         equ   jnl
@@jn_gt         equ   jng
@@jn_le         equ   jnle
@@jn_ge         equ   jnge
@@jn_of         equ   jno
@@jn_ze         equ   jnz
@@jn_nz         equ   jz
@@jn_gz         equ   jng
@@jn_lz         equ   jnl
@@jn_above      equ   jna
@@jn_not_above  equ   ja
@@jn_below      equ   jnb
@@jn_not_below  equ   jb
@@jn_ae         equ   jnae
@@jn_above_or_equal equ   jnae
@@jn_be         equ   jnbe
@@jn_above_or_equal equ   jnbe
@@jn_carry      equ   jnc
@@jn_no_carry   equ   jc
@@jn_overflow   equ   jnc
@@jn_no_overflow equ   jc
@@jn_true       equ   jz
@@jn_false      equ   jnz

@@jn_ecx        equ   jecxz
@@jn_cx         equ   jcxz

use32 = false

D_Call_Real equ 8+3     ; Descriptor mit DPL=3 fÅr CALLs aus Protected Mode
                        ; nach Real Mode. (Modul EmProt)

        @init

        .list
