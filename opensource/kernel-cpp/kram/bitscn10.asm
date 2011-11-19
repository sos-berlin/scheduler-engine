 title In einem Bitfeld eine Folge von Einsen suchen und auf Null setzen
; 31. 3.91                                             (c) Joacim Zschimmer

; Bisher ist nur Anzahl_Einsen = 1 getestet!!  31.3.91
;
; !! Wenn nach einer Doppelwortgrenze festgestellt wird, da· nicht genug
; !! Einsen da sind, werden die bereits gelîschten Einsen nicht wieder gesetzt!!
; !! Lîsung: Diese Routine lîscht die Einsen nicht (bitset kann sie lîschen).

                include jzincl.inc

bitscn10_TEXT   segment dword public use16 'CODE'
                assume  CS : bitscn10_TEXT

                public  bitscn10
                public  _bitscn10

stk_hdr         equ     [ESP+4+4+4+4+2]          ; SP nicht mehr Ñndern!

Bitfeld         equ     stk_hdr + 0
Bitfeld_Laenge  equ     stk_hdr + 4
Anzahl_Einsen   equ     stk_hdr + 8

; Sucht in einem Bitfeld eine anzugebene Anzahl aufeinanderfolgender Einsen
; (von links nach rechts), setzt die gefundenen Einsen auf 0 und gibt die
; Bitposition des ersten Bits im Bitfeld zurÅck.
;
; Wenn das Bitfeld keine so gro·e Gruppe aufeinanderfolgender Einsen enthÑlt,
; wird -1 zurÅckgegeben.
;
; Wenn die gesuchte Anzahl kleiner als 1 ist, wird -2 zurÅckgegeben.

bitscn10 proc    far
_bitscn10:

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

        SUB     EDI,4                   ; EDI = offset des Doppelworts

        MOV     EAX,ES:[EDI]            ; Doppelwort aus der Bitmap <> 0
        SUB     EDX,EDX                 ; Bei Bit-Position 0 beginnen

        BSF     EDX,EAX                 ; Suche ein 1-Bit im Doppelwort

        CMP     EBP,1
        @if eq, then                    ; Nur eine Seite belegen:
            BTR     EAX,EDX                 ; Setze es auf 0
            MOV     dword ptr ES:[EDI],EAX  ; Doppelwort wieder speichern
            JMP     Fertig                  ; Offset in DI, Bitposition in EDX
        @fi

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
Set_0_Loop:                             ; EDI zeigt auf das aktuelle Doppelwort
                                        ; EDX = Position der Einser-Gruppe
                                        ; EBX = Anzahl der benîtigten Einsen
                                        ;       im aktuellen Doppelwort
            MOV     CL,BL               ; Anzahl der benîtigten Einsen
            DEC     CL
            MOV     EAX,0FFFFFFFEh      ; 31 Einsen, eine Null
            SHL     EAX,CL
            NOT     EAX                 ; Soviele Einsen wie in EBX steht

            MOV     CL,DL               ; Position der Gruppe
            SHL     EAX,CL
            NOT     EAX
            AND     ES:[EDI], EAX       ; Bits auf 0 setzen

            SUB     EBP,EBX             ; Anz. der noch auf 0 zu setzenden Bits
           JZ      short Fertig

            MOV     EBX,EBP
            Min     BL,32               ; EBX := min (EBP, 32)
            MOV     DL,32
            SUB     DL,BL               ; Position der Einser-Gruppe

            SUB     EDI,4               ; Vorangehendes Doppelwort
           JMP     Set_0_Loop


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


bitscn10        endp
bitscn10_TEXT   ends
                END
