; xlat - memcpy mit far pointers
; 16. 1.91						    Joacim Zschimmer

        include jzincl.inc

        public	_xlat_asm

xlat_asm_text   segment para use16 'code'
_xlat_asm       proc    far
        assume  CS: xlat_asm_text

fs_allowed = false                  ; In Windows 3.1 offenbar nicht benutzbar

        ENTER   0,0
        PUSH	DI
        PUSH    SI
        PUSH	DS
        PUSH    ES

        if fs_allowed
            PUSH    FS

            LES	DI, SS:[BP+6]           ; Destination
            LFS	SI, SS:[BP+10]          ; Source
            MOV	CX, SS:[BP+14]          ; L„nge
            LDS	BX, SS:[BP+16]          ; XLAT-Tabelle

            @if cx, then
                align   8
                @loop
                    LODS    byte ptr FS:[SI]
                    XLAT    byte ptr DS:[BX]
                    STOS    byte ptr ES:[DI]
                @end_loop
            @fi

            POP     FS

         else

            mov     eax, ss:[bp+6]          ; Destination
            cmp     eax, SS:[BP+10]         ; Source
            @if ne, then
                LES     DI, SS:[BP+6]           ; Destination
                LDS     SI, SS:[BP+10]          ; Source
                MOV     CX, SS:[BP+14]          ; Länge

                mov     al, cl
                shr     cx, 2
                rep movs dword ptr es:[di], dword ptr ds:[si]

                and     al, 3
                mov     cl, al
                rep movs byte ptr es:[di], byte ptr ds:[si]
            @fi

            LDS     SI, SS:[BP+6]           ; String
            LES     DI, SS:[BP+16]          ; XLAT-Tabelle
            MOV     CX, SS:[BP+14]          ; Länge

            @if cx, then
                sub     bx, bx

                align   8
                loop1:
                    mov     bl, ds:[si]         ; laden
                    dec     cx
                    mov     al, es:[di+bx]      ; umsetzen
                    mov     ds:[si], al         ; speichern
                    lea     si, [si+1]
                    jnz     loop1
            @fi

        endif

        POP     ES
        POP     DS
        POP     SI
        POP     DI

        LEAVE
        RET

_xlat_asm	endp
xlat_asm_text   ends
	end

