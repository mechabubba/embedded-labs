;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Assembly language file for Lab 1 in 55:036 (Embedded Systems)
; Spring ECE:3360. The University of Iowa.
;
; LEDs are connected via a 470 Ohm resistor from PB3, PB4 to Vcc
; or GND.
;
; A. Kruger
;
.include "tn45def.inc"
.cseg
.org 0

; Configure PB3 and PB4 as output pins.
more:
      sbi   DDRB,3      ; PB3 is now output
      sbi   DDRB,4      ; PB4 is now output

; Main loop follows.  Toggle PB1 and PB2 out of phase.
; Assuming there are LEDs and current-limiting resistors
; on these pins, they will blink out of phase.

; PB3 is the white LED and PB4 is the blue LED
   loop:
      sbi   PORTB,3     ; LED at PB3 off
      cbi   PORTB,4     ; LED at PB4 on 
      rcall delay_blue  ; Wait with PB4 (blue) on
      cbi   PORTB,3     ; LED at PB3 on
      sbi   PORTB,4     ; LED at PB4 off  
      rcall delay_white  ; Wait with PB3 (white) on
      rjmp  loop

; Generate a delay using three nested loops that does nothing. 
; With a 8 MHz clock, the values below produce ~320 ms delay.

; 310 ms delay for white.
   delay_white:
      ldi   r23,13      ; r23 <-- Counter for outer loop
  w1: ldi   r24,255    ; r24 <-- Counter for level 2 loop 
  w2: ldi   r25,248     ; r25 <-- Counter for inner loop
      nop               ; no operation
  w3: dec   r25
      brne  w3 
      dec   r24
      brne  w2
      dec   r23
      brne  w1

	  ldi r23,84		; Manual tuning with a single-depth loop to get it as close as possible (310.00013 ms)
  w4: nop
	  dec r23
	  brne w4

      ret

; 305 ms delay for blue.
   delay_blue:
      ldi   r23,13      ; r23 <-- Counter for outer loop
  b1: ldi   r24,255     ; r24 <-- Counter for level 2 loop 
  b2: ldi   r25,244     ; r25 <-- Counter for inner loop
      nop               ; no operation
  b3: dec   r25
      brne  b3 
      dec   r24
      brne  b2
      dec   r23
      brne  b1

	  ldi r23,29		; Manual tuning with a single-depth loop to get it as close as possible
  b4: nop
	  dec r23
	  brne b4

      ret
.exit