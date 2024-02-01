;
; Lab02_Tilt.asm
;
; Created: 1/26/2024 9:19:54 AM
; Author : jmulder1
;

.include "tn45def.inc"
.cseg
.org 0

start:			; Initializes the pin states
    cbi DDRB,0	; Set PB0 to input - PB0 is the tilt sensor
	sbi DDRB,3	; Set PB3 to output - PB3 is the green LED
	sbi DDRB,4	; Set PB4 to output - PB4 is the speaker
	rjmp loop	; Enter the main process loop

loop:
	sbis PINB,0	; Skip next instruction if PB0 is high
	rcall alarm	; PB0 is low -> The tilt sensor is not upright
	sbi PORTB,3	; Set the green LED on
	rjmp loop
	
alarm:
	cbi PORTB,3	; Alarm has been triggered - turn off the green LED
	ldi R22,5	; Basically a random guess for a debounce/hold timer
	b1:	rcall buzz
		dec R22
		brne b1
	sbis PINB,0	; Skip next instruction if PB0 is still high
	rjmp alarm	; If PB0 is still high, just loop back on alarm (hopefully this doesn't mess up the return register)
	ret

buzz:	; does 1 cycle of the speaker tone (This probably could be incorporated into the alarm subroutine)
	cbi PORTB,4
	rcall delay
	sbi PORTB,4
	rcall delay
	ret

delay:	; currently set to give roughly 800 Hz
	ldi R20,255	; R20 is a loop counter
	d1:	ldi R21,50
		d2:	dec R21
			brne d2
		dec R20
		brne d1	; Return to d1 until R20 is 0
	ret