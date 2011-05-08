 title In Bitfeld auf nur Einsen oder nur Nullen prÅfen
;  6. 2.89                                        Copyright     Joacim Zschimmer
        include jzincl.inc

bitchk_text segment dword public use16 'CODE'

        assume  CS : bitchk_text

        public  bitchk
        public  _bitchk

@par_d bitfeld
@par_d bit_position
@par_d bit_anzahl
@par_w bit

bitchk  proc    far
_bitchk:

        ENTER   @@local_size,0
        PUSHAD

        CLD

        MOV     EAX,Bit_Position
        MOVZX   EDX,AL
        AND     DL,31         ; Bitposition im Doppelwort

        SHR     EAX,5-2       ; Doppelwortindex (*4)
        AND     AL,11111100b

        SUB     EDI,EDI
        LES     DI, Bitfeld
        ADD     EDI,EAX

        MOV     EBX,Bit_Anzahl

        @repeat                 ; Schleifendurchlauf fÅr jedes Doppelwort
            MOV     ECX,32
            SUB     ECX,EDX
            Min     ECX,EBX       ; Anz. der in diesem Dw. 0 zu setzenden Bits

            MOV     EAX,0FFFFFFFEh; Eine Null
            DEC     CL
            SHL     EAX,CL        ; Anzahl Nullen wie in ECX stand
            XCHG    ECX,EDX       ; Anzahl und Position vertauschen
            ROL     EAX,CL        ; An die richtige Position rotieren

            CMP     Bit,0
            @if eq, then
                NOT     EAX
                TEST    dword ptr ES:[EDI], EAX
                JNZ     short Fehler
             @else
                OR      EAX, dword ptr ES:[EDI]
                NOT     EAX
                JNZ     short Fehler
            @fi

            ADD     EDI,4

            SUB     EBX,EDX       ; EDX = Anz. der freigegebenen Seiten - 1
            MOV     EDX,0         ; FÅr den nÑchsten Schleifendurchlauf: Pos. 0
            DEC     EBX           ; Anzahl der noch freizugebenden Seiten
        @until ze

        POPAD
        SUB     AX,AX
        LEAVE
        RET

Fehler: POPAD
        MOV     AX,-1
        LEAVE
        RET

bitchk  endp
bitchk_text ends
        END

