 title In einem Bitfeld eine Folge von Einsen suchen und auf Null setzen
; 31. 3.91                                             (c) Joacim Zschimmer

                .model  large

                include jzincl.inc

                .data

adresse         dw      ?       
bit_position    dw      ?


;bitscan1_TEXT   segment dword public use16 'CODE'
                .code
;                assume  CS : bitscan1_TEXT

                public  bitscan1
                public  _bitscan1

stk_hdr         equ     [ESP+4+4+4+4+2]          ; SP nicht mehr Ñndern!

Bitfeld         equ     stk_hdr + 0
Bitfeld_Laenge  equ     stk_hdr + 4
Anzahl_Einsen   equ     stk_hdr + 8

; Sucht in einem Bitfeld eine anzugebene Anzahl aufeinanderfolgender Einsen
; (von links nach rechts) und gibt die
; Bitposition des ersten Bits im Bitfeld zurÅck.
;
; Wenn das Bitfeld keine so gro·e Gruppe aufeinanderfolgender Einsen enthÑlt,
; wird -1 zurÅckgegeben.
;
; Wenn die gesuchte Anzahl kleiner als 1 ist, wird -2 zurÅckgegeben.

bitscan1 proc    far
_bitscan1:

        PUSH    EBP                     ; Wird Anzahl_Einsen enthalten
        PUSH    ESI
        PUSH    EDI
        PUSH    ES

        AND     ESP, 0000FFFFh

        CLD

        SUB     EDI, EDI
        LES     DI, Bitfeld

        MOV     EBP, dword ptr Anzahl_Einsen
        OR      EBP, EBP
        JLE     error

; Schleife, wenn das nÑchste Doppelwort <> 0 gesucht werden soll:

Scan_Dwords:
        MOVZX   ECX, word ptr Bitfeld
        MOV     EAX, dword ptr Bitfeld_laenge
        SHR     EAX, 3                  ; LÑnge in Bits
        ADD     ECX, EAX                ; Anzahl der restlichen Bytes

        SUB     ECX, EDI
        JLE     Nicht_da                ; Am Ende angelangt?

        SUB     EAX,EAX                 ; Suche ein Doppelwort <> 0

        SHR     ECX, 2                  ; Doppelwortweise
        REPE SCAS dword ptr ES:[DI]
        JE      Nicht_da

        SUB     DI,4                    ; DI = offset des Doppelworts

        MOV     EAX,ES:[EDI]            ; Doppelwort aus der Bitmap <> 0
        SUB     EDX,EDX                 ; Bei Bit-Position 0 beginnen

        BSF     EDX,EAX                 ; Suche ein 1-Bit im Doppelwort

        CMP     EBP,1                   ; Nur eine Seite belegen:
        JE      Fertig

        MOV     adresse, DI
        MOV     bit_position, DX
                                        ; Mehrere Seiten belegen
                                        ; Bit-Position der 1. Eins ist in EDX
        MOV     EBX,EBP                 ; Anzahl der Einsen
        MOV     CL,DL
        SHR     EAX,CL                  ; Gefundene 1 ist jetzt rechtsbÅndig

Find_0_Loop:                            ; Falls Seiten Åber mehrere DWorte gehen
            SHR     EAX,1              ; Jetzt ist die 1 weg (links ist eine 0)

            NOT     EAX                ; Suche nÑchste 0 im Doppelwort
            BSF     ECX,EAX            ; ECX := Anzahl der Einsen - 1
            SHR     EAX,CL             ; Gefundene Einsen hinausschieben
            INC     CL                 ; ECX := Anzahl der Einsen

            CMP     ECX,EBX            ; Reicht's?
           JGE     Gefunden

            ADD     DL,CL
            CMP     DL,32
            JNE     short Next_1       ; Nein: nÑchstes 1-Bit suchen

            SUB     EBX,ECX            ; Um die Anzahl der Einsen verringern

            ADD     EDI,4              ; NÑchstes Doppelwort
            MOV     EAX,ES:[EDI]

            SUB     DL,DL              ; Bei Position 0 beginnen

            TEST    AL,1               ; RechtsbÅndige 1?
           JNZ     Find_0_Loop

Next_1:                                 ; Die nÑchste Gruppe von Einsen suchen
            MOV     EBX,EBP             ; Anzahl der anzufordernden Seiten

            BSF     ECX,EAX             ; NÑchste 1 suchen
           JNZ     Scan_DWords          ; Keine da? NÑchstes DWort <> 0 suchen

            MOV     DL,CL               ; Bit-Position der 1
            SHR     EAX,CL              ; Gefundene 1 an den rechten Rand
           JMP     Find_0_Loop

Gefunden:
        MOV     DI, adresse
        MOV     DX, bit_position


Fertig:
        SUB     DI,word ptr Bitfeld     ; Nummer des Bytes in der Bitmap
        LEA     EAX,[EDI*8+EDX]         ; * 8 + Nummer des Bits

Return: MOV     EDX,EAX
        SHR     EDX,16
        AND     EAX,0000FFFFh

        SUB     EBX,EBX                 ; 32-Bit-Register auf 0 setzen!
        MOV     ECX,EBX

        POP     ES
        POP     EDI
        POP     ESI
        POP     EBP

        RET

;-------------------------------------------------------------------------------

Nicht_da:                               ; Speicher nicht verfÅgbar
        MOV     AX,-1
        JMP     Return

error:
        MOV     AX,-2                   ; Operandenfehler
        JMP     Return


bitscan1        endp

;bitscan1_TEXT   ends
                END

