 title In einem Bitfeld eine Folge auf Null oder Eins setzen
; 18. 9.88                                        Copyright     Joacim Zschimmer
          include jzincl.inc

bitset_text segment dword public use16 'CODE'

          assume  CS : bitset_text

          public  bitset
          public  _bitset

@par_d Bitfeld
@par_d Bit_Position
@par_d Bit_Anzahl
@par_w Bit

bitset  proc    far
_bitset:

        ENTER   @@local_size,0
        PUSH    EDI
        PUSH    ES

        CLD

        MOV     EAX,Bit_Position
        MOVZX   EDX,AL
        AND     DL,31         ; Bitposition im Doppelwort

        SHR     EAX,5-2       ; Doppelwortindex (*4)
        AND     AL,11111100b

        LES     DI, Bitfeld
        ADD     DI, AX

        MOV     EBX,Bit_Anzahl

        @repeat                  ; Schleifendurchlauf fÅr jedes Doppelwort
            MOV     ECX,32
            SUB     ECX,EDX
            Min     ECX,EBX       ; Anz. der in diesem Dw. auf 0 zu setzenden Bits

            MOV     EAX,0FFFFFFFEh; Eine Null
            DEC     CL
            SHL     EAX,CL        ; Anzahl Nullen wie in ECX stand
            XCHG    ECX,EDX       ; Anzahl und Position vertauschen
            ROL     EAX,CL        ; An die richtige Position rotieren

            CMP     Bit,0
            @if eq, then
                AND     dword ptr ES:[DI], EAX   ; Bits auf 0 setzen
             @else
                NOT     EAX
                OR      dword ptr ES:[DI], EAX   ; Bits auf 1 setzen
            @fi

            ADD     DI,4

            SUB     EBX,EDX       ; EDX = Anz. der freigegebenen Seiten - 1
            MOV     EDX,0         ; FÅr den nÑchsten Schleifendurchlauf: Pos. 0
            DEC     EBX           ; Anzahl der noch freizugebenden Seiten
        @until ze

        SUB     EAX,EAX       ; 32-Bit-Register lîschen
        MOV     EBX,EAX
        MOV     ECX,EAX
        MOV     EDX,EAX

        POP     ES
        POP     EDI
        LEAVE
        RET

bitset  endp
bitset_text ends
        END

