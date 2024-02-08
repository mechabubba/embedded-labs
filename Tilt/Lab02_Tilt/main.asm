;
; Lab02_Tilt.asm
;
; Created: 1/26/2024 9:19:54 AM
; Author : jmulder1, vanni
;

.include "tn45def.inc"
.cseg
.org 0

; Initializes the pin states
start:
    cbi DDRB,0	; Set PB0 to input - PB0 is the tilt sensor
	sbi DDRB,3	; Set PB3 to output - PB3 is the green LED
	sbi DDRB,4	; Set PB4 to output - PB4 is the speaker
	rjmp loop	; Enter the main process loop

; Main program loop.
loop:
	sbis PINB,0		; Skip next instruction if PB0 is high
	rcall alarm_on	; PB0 is low -> The tilt sensor is not upright
	sbic PINB,0		; Skip next instruction if PB0 is high
	rcall alarm_off	; PB0 is high -> The tilt sensor is upright
	rjmp loop

; Activates the speakers and LED.
alarm_on:				; 5 cycles of the on tone for the alarm being on, before checking again.
	cbi PORTB,3			; Clear the green LED pin.
	ldi R22,2
	a1:	cbi PORTB,4		; Clear the speaker pin.
		rcall delay
		sbi PORTB,4		; Set the speaker pin.
		rcall delay		; These 2 commands along with the delays will cause the speaker to emit a tone.
		dec R22			; These 2 operations will push the time off by a very small amount.
		brne a1
	ret

; Complement of the alarm_on subroutine; doesn't activate speakers or LED, but takes the same amount of cycles.
alarm_off:
	sbi PORTB,3			; Set the green LED pin
	ldi R22,2
	b1:	nop				; NOP replaces the speaker pin sbi/cbi so that the timing is the same
		rcall delay
		nop
		rcall delay
		dec R22			; These 2 operations will push the time off by a very small amount
		brne b1
	ret

; Delay subroutine set to give roughly 205 kHz
delay:
	ldi R20,6			; R20 is the outer loop counter.
	d1:	ldi R21,107
		d2:	dec R21		; R21 is the inner loop counter. This loops (107 * 6) times.
			brne d2		; When R21 != 0, branch to d2.
		dec R20
		brne d1			; When R20 != 0, branch to d1.
	ret

.exit
