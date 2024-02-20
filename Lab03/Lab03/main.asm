;
; Lab03.asm
;
; Created: 2/6/2024 3:30:42 PM
; Author : jared
;

.include "tn45def.inc"

; macro small macro to write 7seg digits to a register.
; @0 is register compared, @1 is register written to.
.macro segwrite
	cpi @0,0 ; digit=0
	brne d0
	ldi @1,0xFC
d0:	cpi @0,1 ; digit=1
	brne d1
	ldi @1,0x60
d1:	cpi @0,2 ; digit=2
	brne d2
	ldi @1,0xDA
d2:	cpi @0,3 ; digit=3
	brne d3
	ldi @1,0xF2
d3:	cpi @0,4 ; digit=4
	brne d4
	ldi @1,0x66
d4:	cpi @0,5 ; digit=5
	brne d5
	ldi @1,0xB6
d5:	cpi @0,6 ; digit=6
	brne d6
	ldi @1,0xBE
d6:	cpi @0,7 ; digit=7
	brne d7
	ldi @1,0xE0
d7:	cpi @0,8 ; digit=8
	brne d8
	ldi @1,0xFE
d8:	cpi @0,9 ; digit=9
	brne d9
	ldi @1,0xF6
d9:
.endmacro

.cseg
.org 0

; R16 will be the state ID register: 0 -> set R28 to 0, 1 -> increment R28, 2 -> hold R28 at value (either pause or overflow)
; R28 will be the counter, split between the upper and lower nibble. The lower nibble will store the 1s place (0-9),
; and the upper nibble will store the 10s place (0-10). If the upper nibble reads 10, then overflow has occurred.
; R29 will be the LED data for the leftmost segment
; R30 will be the LED data for the rightmost segment

init:
	; Set up data direction registers
	sbi DDRB,0
	sbi DDRB,1
	sbi DDRB,2
	cbi DDRB,3
	sbi DDRB,4

loop:
	rcall debounce ; first debounce input
	; based on the result of this (R20) we will set the state accordingly.
	cpi R16,0
	brie state0 ; If R16 = 0, go to state0.
	cpi R16,1
	brie state1 ; If R16 = 1, go to state1.
	rjmp write_segments ; Otherwise head straight to the segment encoder

debounce:
	; reimplemented from class code...
	; r21 is num of 0s, r22 is num of 1s. r23 is loop index
	; r20 is 1 if buttons pressed, 0 otherwise.
	clr R21 ; clear some regs beforehand.
	clr R22
	ldi R23, (10 - 1)

	; the loop.
	_db:
		sbic PORTB,3	; if PB3 is clear (button unpressed)...
		inc	R21			; inc it
		sbis PORTB,3	; if PB3 is set (button pressed)...
		inc R22			; inc it

		dec R23			; loop logic over. dec index
		cpi R23, 0		; compare index to 0...
		breq _db_check_results	; ...and branch out if we need to.

	; case *after* loop.
	_db_check_results:
		cp R22, R21
		brge _db_done_le	; if R22 >= R21 (if more 1s than 0s), buttons assumed to be pressed.
		ldi R20, 1			; set 1 and get outta here
		rjmp _db_done

		_db_done_le:
			ldi R20, 0		; otherwise, assumed to be not pressed.
			rjmp _db_done

	_db_done:
		ret ; able to ret bc of rcall

; state zero clears the counter and writes to the display.
state0:
	clr R28
	rjmp write_segments

; state 1 
state1:
	inc R28
	mov R17,R28
	andi R17,0x0F
	cpi R17,0x0A
	brlo write_segments ; Goto write_segments if we have not exceeded 9 on the lower digit
	andi R28,0xF0 ; Clear the lower digit to 0
	subi R28,0xF0 ; Add 1 to the upper nibble by subtracting -16, the fact that there is no addi is really stupid
	rjmp write_segments

write_overflow:
	ldi R29,0xFD
	ldi R30,0x8F
	rjmp write_IO

write_segments: ; Converts the number in R28 into the 7-segment encodings for R29,R30
	mov R17,R28
	swap R17
	andi R17,0x0F ; get the upper nibble of R28 first
	cpi R17,10
	breq write_overflow ; If counter is overflowing then write OF instead.
	segwrite R17,R29 ; Write the left digit
	sbr R29,0x01 ; Add the decimal point to the left digit
	mov R17,R28
	andi R17,0x0F ; Then get the lower nibble of R28
	segwrite R17,R30 ; Write the right digit
	rjmp write_IO

write_IO:
	; SEND TO PINS HERE
	; we will need to do the clock manually here. bits shifted in lsb to msb, R29 first, then R30.
	.macro send_bit
		
	.endmacro
	rjmp loop

