;
; Lab03.asm
;
; Created: 2/6/2024 3:30:42 PM
; Author : jared
;

.include "tn45def.inc"
.cseg
.org 0

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
	


rst_counter:
	

	
