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
	rcall alarm_on	; PB0 is low -> The tilt sensor is not upright
	sbic PINB,0	; Skip next instruction if PB0 is high
	rcall alarm_off	; PB0 is high -> The tilt sensor is upright
	rjmp loop

alarm_on:	; 5 cycles of the on tone for the alarm being on, before checking again.
	cbi PORTB,3	; Clear the green LED pin
	ldi R22,2
	a1:	cbi PORTB,4
		rcall delay
		sbi PORTB,4
		rcall delay
		dec R22			; These 2 operations will push the time off by a very small amount
		brne a1
	ret

alarm_off:	; Complement of the alarm_on subroutine, does not activate speakers but takes the same amount of time.
	sbi PORTB,3	; Set the green LED pin
	ldi R22,2
	b1:	nop				; NOP replaces the speaker pin sbi/cbi so that the timing is the same
		rcall delay
		nop
		rcall delay
		dec R22			;These 2 operations will push the time off by a very small amount
		brne b1
	ret

delay:	; Set to give roughly 205 kHz
	ldi R20,6		; R20 is a loop counter
	d1:	ldi R21,107
		d2:	dec R21
			brne d2
		dec R20
		brne d1	; Return to d1 until R20 is 0
	ret

.exit