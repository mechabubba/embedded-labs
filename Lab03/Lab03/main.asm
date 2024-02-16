;
; Lab03.asm
;
; Created: 2/6/2024 3:30:42 PM
; Author : jared
;

.include "tn45def.inc"
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
	; TODO: Add button debounce code here
	cpi R16,0
	brie state0 ; If R16 = 0, go to state0.
	cpi R16,1
	brie state1 ; If R16 = 1, go to state1.

write_segments: ; Converts the number in R28 into the 7-segment encodings for R29,R30
	mov R17,R28
	swap R17
	andi R17,0x0F ; check the upper nibble of R28 first	 



state0:
	clr R28
	rjmp write_segments

state1:
	inc R28
	mov R17,R28
	andi R17,0x0F
	cpi R17,0x0A
	brlo write_segments ; Goto write_segments if we have not exceeded 9 on the lower digit
	andi R28,0xF0 ; Clear the lower digit to 0
	add R28,0x10 ; Add 1 to the upper nibble
	rjmp write_segments
