;
; Lab03.asm
;
; Created: 2/6/2024 3:30:42 PM
; Author : Jared M., Steven V.
; Description: Implemetation of an interval timer using two seven segment displays.
;   On button push, two digit timer will start to increase until button is pressed again, where the user can press it once more to reset to zero.
;   If timer value surpasses 9.9, the display will show an overflow error, where the user can again reset to zero.

.include "tn45def.inc"

; """small""" macro to write 7seg digits to a register.
; @0 is register compared, @1 is register written to.
.macro segwrite
	cpi @0,0 ; digit=0
	brne d0
	ldi @1,0b11111100
d0:	cpi @0,1 ; digit=1
	brne d1
	ldi @1,0b01100000
d1:	cpi @0,2 ; digit=2
	brne d2
	ldi @1,0b11011010
d2:	cpi @0,3 ; digit=3
	brne d3
	ldi @1,0b11110010
d3:	cpi @0,4 ; digit=4
	brne d4
	ldi @1,0b01100110
d4:	cpi @0,5 ; digit=5
	brne d5
	ldi @1,0b10110110
d5:	cpi @0,6 ; digit=6
	brne d6
	ldi @1,0b10111110
d6:	cpi @0,7 ; digit=7
	brne d7
	ldi @1,0b11100000
d7:	cpi @0,8 ; digit=8
	brne d8
	ldi @1,0b11111110
d8:	cpi @0,9 ; digit=9
	brne d9
	ldi @1,0b11110110
d9: cpi @0,10 ; '-'
	brlo dA
	ldi @1,0b00000010
dA:	
.endmacro

.cseg
.org 0

; R16 will be the state ID register: 0 -> set R28 to 0, 1 -> increment R28, 2 -> hold R28 at value (either pause or overflow)
; R20 holds status of button. bit 1 is current button pin status, bit 2 is old button pin status (note: button pressed means pin is low).
; R28 will be the counter, split between the upper and lower nibble. The lower nibble will store the 1s place (0-9),
; and the upper nibble will store the 10s place (0-10). If the upper nibble reads 10, then overflow has occurred.
; R29 will be the LED data for the leftmost segment
; R30 will be the LED data for the rightmost segment

; init.
init:
	; Set up data direction registers
	sbi DDRB,0
	sbi DDRB,1
	sbi DDRB,2
	cbi DDRB,3
	sbi DDRB,4
	ldi R16,0
	ldi R20,3

; main loop.
loop:
	rcall debounce ; first debounce input. based on the result of this (R20) we will change the state accordingly.
	cpi R16,0
	breq state0	; If R16 = 0, go to state0.
	cpi R16,1
	breq state1	; If R16 = 1, go to state1.
	cpi R16,3
	breq state3	; If R16 = 3, go to state3.

	; Otherwise, go to State 2:
	cpi R20,1
	brne loop ; If button is not just unpressed, go to encoder.
	ldi R16,0 ; Set state back to 0 then go to encoder
	rjmp loop

; debounce subroutine reimplemented from class code.
; R21: num of 0s. R22: num of 1s. R23: loop index
; sets R20 on exit.
debounce:
	clr R21 ; clear some regs beforehand.
	clr R22
	ldi R23, 10

	; the loop.
	_db:
		sbic PINB,3	; if PB3 is clear (button unpressed)...
		inc	R21		; inc it
		sbis PINB,3	; if PB3 is set (button pressed)...
		inc R22		; inc it

		dec R23		; loop logic over. dec index
		brne _db

	lsl R20			; shift R20 once to the left so the second bit stores the stat
	andi R20,0x3	; Make sure that the upper 6 bits are not affected.
	cp R22, R21
	brge _db_done_le	; if R22 >= R21 (if more 1s than 0s), buttons assumed to be pressed.
	sbr R20, 1			; set 1 and get outta here
	rjmp _db_done

	_db_done_le:
		cbr R20,1		; otherwise, assumed to be not pressed.
		rjmp _db_done

	; we're done.
	_db_done:
		ret ; able to ret bc of rcall

; state zero clears the counter and writes to the display.
state0:
	clr R28
	cpi R20,1			; Was the button just unpressed?
	brne write_segments ; If it was, then don't change the state
	ldi R16,1			; Switch to state 1
	rjmp write_segments

; state 1.
; note: r28 stores both digits as each respective nibble. 
state1:
	cpi R20,2	; Is button just pressed?
	brne s1		; If its not, don't change state. 
	ldi R16,3
s1:	inc R28
	mov R17,R28
	andi R17,0x0F
	cpi R17,0x0A
	brlo write_segments	; Goto write_segments if we have not exceeded 9 on the lower digit
	andi R28,0xF0		; Clear the lower digit to 0
	subi R28,0xF0		; Add 1 to the upper nibble by subtracting -16, the fact that there is no addi is really stupid
	cpi R28,0xA9 
	brlo write_segments	; Skip if less than 100
	ldi R16,3			; Switch to state 2
	rjmp write_segments	

; state 3. do nothing until next press.
; "wheres state 2?" we didn't want to jump around and change the expected numbers for R16. this is how it must be.
state3:
	cpi R20,2
	brne write_segments
	ldi R16,2 ; Goto state 2 when button is pressed
	rjmp write_segments

; writes 'O.F' to signify overflow.
write_overflow:
	ldi R29,0b11111101
	ldi R30,0b10001111
	rjmp write_IO

; Converts the number in R28 into the 7-segment encodings for R29,R30
write_segments:
	mov R17,R28
	swap R17
	andi R17,0x0F ; get the upper nibble of R28 first
	cpi R17,10
	brsh write_overflow ; If counter is overflowing then write OF instead.
	
	; Otherwise do normal write:
	segwrite R17,R29 ; Write the left digit
	sbr R29,0x01 ; Add the decimal point to the left digit
	mov R17,R28
	andi R17,0x0F ; Then get the lower nibble of R28
	segwrite R17,R30 ; Write the right digit
	rjmp write_IO

; write to io. send bits from r29 and r30. uses a handy macro defined above.
write_IO:
	sbi PORTB,0	; pull OE high. needs to stay high for a vague number of cycles above 1. this should be enough.

	; Send 10s byte
	ldi R23, 8
	_sb_shift_loop_1:	; Lower digit
		sbrc R30,0		; send the rightmost bit
		sbi PORTB,4
		sbrs R30,0
		cbi PORTB,4
		sbi PORTB,2		; clock in
		cbi PORTB,2		; clock out
		lsr R30			; lsl so the next bit is the rightmost
		dec R23			; Decrement the loop counter
		brne _sb_shift_loop_1
	cbi PORTB,4	; reset pin 4 here, because things go very wrong if we dont

	; Send 1s byte
	ldi R23, 8
	_sb_shift_loop_2:	; Upper digit
		sbrc R29,0		; send the rightmost bit
		sbi PORTB,4
		sbrs R29,0
		cbi PORTB,4
		sbi PORTB,2		; clock in
		cbi PORTB,2		; clock out
		lsr R29			; logical shift left so the next bit is the rightmost
		dec R23			; Decrement the loop counter
		brne _sb_shift_loop_2

	sbi PORTB,1	; pulse latch
	cbi PORTB,1
	cbi PORTB,0	; pull OE low

	; Loop to drag out the timer to be closer to 1 ms
	ldi R23,4
	i1:
		ldi R24,255
		i2:
			ldi R25,255
			i3:
				dec R25
				brne i3
			dec R24
			brne i2
		dec R23
		brne i1
	rjmp loop ; return to freakin sender

