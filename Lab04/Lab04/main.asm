;
; Lab04.asm
;
; Created: 2/27/2024 11:02:18 AM
; Authors: Steven Vanni, Jared Mulder
; Description: Lab04 displays words on an LCD.
;

.include "m328Pdef.inc"

.cseg
.org 0

init:
	sbi DDRB,3 ; pb5 (enable) is output
	sbi DDRB,5 ; pb3 (r/w select) is output
	sbi DDRC,0 ; \
	sbi DDRC,1 ; | data lines
	sbi DDRC,2 ; | are output
	sbi DDRC,3 ; /
	rjmp loop

loop:
    inc r16
