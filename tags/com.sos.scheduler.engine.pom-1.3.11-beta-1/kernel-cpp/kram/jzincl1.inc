; Makros fÅr "strukturierte" Programmierung in Assembler
; 16. 9.88							Joacim Zschimmer
if1
.xlist
.xcref


@init	  macro

	  if1
	     ifdef @@init1
		exitm
	     endif
	     @@init1 = 1
	   else
	     ifdef @@init2
		exitm
	     endif
	     @@init2 = 1
	  endif

@@if_sp     = 0 	     ; @if Stack Pointer
@@sp	    = 0 	     ; Stack Pointer fÅr Verschachtelungen
@@label_ctr = 0 	     ; Label Counter
@@label     = 0 	     ; FÅr das erste @@push_label
@@stackorg  = 0              ; FÅr @stack_ und @par_
@@stack_par = 0              ; 0: @stack_   1: @par_
@@par_size  = 0
@@local_size = 0
          endm


@source   macro  label,operation,parameter
label	  operation parameter
	  endm


@@push_label macro sp

	  @@stack_&sp = @@label 	 ; Aktuelles Label merken
	  @@sp = @@sp + 1

	  @@label_ctr = @@label_ctr + 1  ; Neues Label erzeugen
	  @@label     = @@label_ctr

	  endm


@@pop_label macro

	  @@sp = @@sp - 1
	  @@pop_label2 %@@sp

	  endm


@@pop_label2 macro sp

	  @@label = @@stack_&sp

	  endm


@@jump	  macro not, bedingung, label, near?

	  ifb <bedingung>
	     ifnb <near?>
                 jum       @L_&label
	       else
        jmp     short @L_&label
	     endif
	   else
	     ifb <not>
		ifnb <near?>
	@@j_&bedingung @L_&label
		 else
	@@j_&bedingung short @L_&label
		endif
	      else
		ifnb <near?>
	@@jn_&bedingung	@L_&label
		 else
	@@jn_&bedingung short @L_&label
		endif
	     endif
	  endif

	  endm


@@loop	macro bedingung, label

	loop&bedingung @L_&label

	endm


@@set_label macro label
@L_&label:
	   endm


@if	  macro bedingung, then

	  @@push_label %@@sp
	  @@jump not, bedingung, %@@label_ctr

	  ifnb <then>
	     @then
	  endif

	  endm


@then	  macro
	  endm


@else	  macro

	  @@merk_label = @@label
	  @@label_ctr = @@label_ctr + 1     ; Neues Label erzeugen
	  @@label     = @@label_ctr

	  @@jump  ,, %@@label
	  @@set_label %@@merk_label

	  endm


@fi	  macro

	  @@set_label %@@label
	  @@pop_label

	  endm


@loop	  macro

	  @@push_label %@@sp
	  @@set_label %@@label

	  endm


@end_loop macro  bedingung

	  @@loop bedingung, %@@label
	  @@pop_label

	  endm


@repeat   macro

	  @@push_label %@@sp
	  @@set_label %@@label

	  endm


@until	  macro  bedingung

	  @@jump not, bedingung, %@@label, back
	  @@pop_label

	  endm


@cycle    macro

	  @@push_label %@@sp
	  @@set_label %@@label

	  endm


@end_cycle macro  bedingung

          @@jump , , %@@label, back
	  @@pop_label

	  endm


@@Get_Reg_XHL macro e?x, rX, rH, rL

        ifidni <e?x>,<EAX>
            rX equ AX
            rH equ AH
            rL equ AL
            exitm
        endif

        ifidni <e?x>,<EBX>
            rX equ BX
            rH equ BH
            rL equ BL
            exitm
        endif

        ifidni <e?x>,<ECX>
            rX equ CX
            rH equ CH
            rL equ CL
            exitm
        endif

        ifidni <e?x>,<EDX>
            rX equ DX
            rH equ DH
            rL equ DL
            exitm
        endif

        .err
        %out Register e?x ?  Nur die Register EAX, EBX, ECX und EDX sind erlaubt
        rX equ <&Register&?X?>
        rH equ <&Register&?H?>
        rL equ <&Register&?L?>

        endm


@stackorg macro reladr			; Setzt relative Adresse fÅr @stackx
@@stackorg = reladr
	  endm


@based_var macro   name, type, segreg, basereg, reladr
          ifnb <name>
            ifdif <name>, <?>
name            EQU     type ptr segreg:[basereg+reladr]
&name&_rel      EQU     reladr
            endif
          endif
	  endm


@par_d    macro   name
          ife @@stack_par
             @@stackorg = 2+4           ; far call!
             @@par_size = 0
             @@local_size = 0
             @@stack_par = true
          endif
          @based_var name, dword, SS, BP, %@@stackorg
          @@stackorg = @@stackorg + 4
          @@par_size = @@par_size + 4
	  endm


@par_w    macro   name
          ife @@stack_par
             @@stackorg = 2+4           ; far call!
             @@par_size = 0
             @@local_size = 0
             @@stack_par = true
          endif
          @based_var name,  word, SS, BP, %@@stackorg
          @@stackorg = @@stackorg + 2
          @@par_size = @@par_size + 2
	  endm


@stack_d  macro   name
          if @@stack_par
             @@stackorg = 0
             @@local_size = 0
             @@stack_par = false
          endif
          @based_var name, dword, SS, BP, %@@stackorg
          @@stackorg = @@stackorg - 4
          @@local_size = @@local_size - 4
	  endm


@stack_w  macro   name
          if @@stack_par
             @@stackorg = 0
             @@local_size = 0
             @@stack_par = false
          endif
          @based_var name,  word, SS, BP, %@@stackorg
          @@stackorg = @@stackorg - 2
          @@local_size = @@local_size - 2
	  endm


Umdreh	macro   e?x
	local   rX, rH, rL

 ifndef i486
	@@Get_Reg_XHL e?x, rX, rH, rL
	XCHG	rH,rL
	ROR	e?x,16
	XCHG	rH,rL
 else
        ifidni <e?x>, <EAX>
        db      0Fh, 0C8h + 0                   ; bswap eax
        else
        ifidni <e?x>, <ECX>
        db      0Fh, 0C8h + 1                   ; bswap ecx
        else
        ifidni <e?x>, <EDX>
        db      0Fh, 0C8h + 2                   ; bswap edx
        else
        ifidni <e?x>, <EBX>
        db      0Fh, 0C8h + 3                   ; bswap ebx
        else
            .error  Unbekanntes Register: umdreg e?x
        endif
        endif
        endif
        endif
 endif
	endm


Max	macro   a,b
	local   l
	CMP	a,b
	JG	short l
	MOV	a,b
l:
	endm

Min	macro   a,b
	local   l
	CMP	a,b
	JL	short l
	MOV	a,b
l:
	endm


umax    macro   a,b
	local   l
	CMP     a,b
        JA      short l
	MOV	a,b
l:
	endm

umin    macro   a,b
	local   l
	CMP	a,b
        JB      short l
	MOV	a,b
l:
	endm

     
CALLr   macro   routine                 ; Call Real aus Protected Mode

routine&_TEXT   segment public use16 'CODE'
                extrn   routine : near
routine&_TEXT   ends

        DB      68h                     ; PUSH imm32
;       DD      offset routine
        DD      offset routine&_TEXT : routine
        ORG     $-2
;       DW      seg routine
        DW      routine&_TEXT
  
        DB      9Ah                     ; CALL ptr16:32
        if use32
            DW      0
        endif
        DW      0, D_Call_Real

        endm


CALLrc  macro   croutine                ; Call Real C aus Protected Mode

        extrn  _&croutine : far

        DB      68h                     ; PUSH imm32
        DD      offset _&croutine
        ORG     $-2
        DW      seg _&croutine

        CALLr   callcc

        endm


CALLc   macro   routine                 ; Call C-Unterprogramm

        extrn   routine : far

        PUSH    seg _&routine
        PUSH    offset _&routine
        CALL    callcc

        endm


Ton     macro   frequenz, msec, pushad

	ifnb <pushad>
	    PUSHAD
	endif    	

        IN      AL,97
        AND     AL,0FCh

        a = frequenz*msec/1000
        if a eq 0
           a = 1
        endif
        MOV     ECX,a

        @loop
           SUB     AL,2
           OUT     97,AL
           MOV     EDX,_CPU_Taktfrequenz/11/frequenz/2
           @repeat              ; 11/12 Zyklen pro Durchlauf
              SUB     EDX,1
           @until ze

           ADD     AL,2
           OUT     97,AL
           MOV     EDX,_CPU_Taktfrequenz/11/frequenz/2
           @repeat              ; 11/12 Zyklen pro Durchlauf
              SUB     EDX,1
           @until ze
        @end_loop

	ifnb <pushad>
	    POPAD
	endif

        endm

JUM     macro   sprungziel
        if use32
        DB      0E9h                    ; JMP near
        DD      sprungziel-($+4)
         else
        jmp     sprungziel
        endif
        endm

JUM2    macro   sprungziel
        if use32
        DB      068h                    ; PUSH imm32
        DD      sprungziel
        RETN
         else
        jmp     sprungziel
        endif
        endm

flush_instruction_queue macro
        local l
        JMP     short l
l:
        endm


@Display macro  text
        local l
        local t

        jmp     short l
t       db      text
        db      '$'
l:      push    ds
        push    cs                      ; Codesegment mu· lesbar sein
        pop     ds
        mov     edx, offset CS: t
        @DispStr dx
        pop     ds

        endm

endif

@movzx  macro   ex, l
        local   aaX, aaH, aaL

        @@Get_Reg_XHL ex, aaX, aaH, aaL

        sub     ex, ex
        mov     aal, l

        endm

@lods   macro   adr, idx, size
        ifidn <processor>,<386>
        LODS    adr
        else
        MOV     EAX, adr
        ADD     idx, size
        endif
        endm

@stos   macro   adr, idx, size
        ifidn <processor>,<386>
        STOS    adr
        else
        MOV     adr, EAX
        ADD     idx, size
        endif
        endm

.list
