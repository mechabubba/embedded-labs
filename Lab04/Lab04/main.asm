;
; Lab04.asm
;
; Created: 2/27/2024 11:02:18 AM
; Authors: Steven Vanni, Jared Mulder
; Team: Team S & J
; Description: Lab04 displays words on an LCD.
;

; Words: Artificial, Intelligence, Robotics, Machine, Learning, Neural

.include "m328Pdef.inc"

.equ line1 = 0x00 ; Labels for setting the LCD cursor to write on the correct line.
.equ line2 = 0x40

.cseg
.org 0

; (R30,R31) is the Z register, for accessing the words in program memory.
; (R28,R29) is the Y register, which lags behind the Z register for the other text line.
; R0 is the register used to store values accessed by the Z register.
; R27 is the counter used to track what word is being displayed.

; R16 is a temporary register used in the display_string subroutine.

words: ; Spaces are included so that the byte count is even, since we are working in a 16-bit instruction processor
	.DB "Press to Scroll",0x00 ; 16 bytes, This one should not reappear in the scrolling sequence
words_loop: ; Point to send Z to on looping.
	.DB "Artificial ",0x00 ; 12 bytes
	.DB "Intelligence ",0x00 ; 14 bytes
	.DB "Robotics ",0x00 ; 10 bytes
	.DB "Machine",0x00 ; 8 bytes
	.DB "Learning ",0x00 ; 10 bytes
	.DB "Neural ",0x00 ; 8 bytes
; The 0x00 following each string is a null character being added as the final byte, making the strings even and serving as a sentinel character like in C.
; There are 7 words, including the "Press to Scroll" statement. Therefore the index will start at 0, and loop to 1 after going to 6.

init:
	sbi DDRB,3 ; pb5 (enable) is output
	sbi DDRB,5 ; pb3 (register select) is output
	sbi DDRC,0 ; \
	sbi DDRC,1 ; | data lines
	sbi DDRC,2 ; | are output
	sbi DDRC,3 ; /
	sbi DDRC,5 ; pc5 (LED) is output
	cbi DDRD,2 ; pd2 (Pushbutton) is input

	sbi PORTC,5 ; Turn LED off until initialization is complete.

	ldi R27,0 ; Set the string counter to 0.
	ldi R28,LOW(2*words) ; Set the Y-register to the "words" label.
	ldi R29,2*HIGH(words)
	ldi R30,LOW(2*words_loop) ; Set the Z-register to the "words_loop" label.
	ldi R31,2*HIGH(words_loop)

	; Initializing the LCD Display
	cbi PORTB,5 ; Setting the LCD to command mode.
	in R16,PINC
	cbr R16,0x0F
	andi R16,0x03
	out PORTC,R16 ; Send 8-bit command to LCD
	rcall delay_5ms ; Wait 5 ms.

	in R16,PINC
	cbr R16,0x0F
	andi R16,0x03
	out PORTC,R16 ; Send 8-bit command to LCD
	rcall delay_100us ; Wait 200 us.
	rcall delay_100us

	in R16,PINC
	cbr R16,0x0F
	andi R16,0x03
	out PORTC,R16 ; Send 8-bit command to LCD
	rcall delay_100us ; Wait 200 us.
	rcall delay_100us

	in R16,PINC
	cbr R16,0x0F
	andi R16,0x02
	out PORTC,R16 ; Send 4-bit command to LCD
	rcall delay_100us ; Wait 200 us.
	rcall delay_100us

	; INSERT REST OF INITIALIZATION HERE!

	rjmp loop

loop:

cycle_words:
	mov R28,R30 ; Copy the Z-register onto the Y-register.
	mov R29,R31
	cpi R27,6
	breq _cycle_back ; If the word index is at 6, cycle back to 0 instead of incrementing.
	inc R27 ; Advance the word index counter.
	adiw Z,1 ; Advance the Z-index past the null character to the next word.
	ret ; Exit cycle_words subroutine (without running _cycle_back).
_cycle_back:
	ldi R27,1 ; Set the word index counter to 1.
	ldi R30,LOW(2*words_loop) ; Move the Z-index to the first word in the loop.
	ldi R31,2*HIGH(words_loop)
	ret ; Exit cycle_words subroutine.

; Update the display every time that cycle_words is executed.
update_display:
	

; Sends the text pointed to by the Y-register to the display (Y lags behind Z so Y will display what Z did on the last cycle).
display_line1:
	lpm R0,Y+ ; Load byte into R0.
	tst R0 ; Does R0 = 0x00 (Null character)?
	breq _end ; If so then exit the loop.
	rcall send_byte
	rjmp display_line1 ; Loop back to start

; Sends the text pointed to by the Z-register to the display.
display_line2:
	lpm R0,Z+ ; Load byte into R0.
	tst R0 ; Does R0 = 0x00 (Null character)?
	breq _end ; If so then exit the loop.
	rcall send_byte
	rjmp display_string ; Loop back to start
_end:
	ret ; Exit display_line1 or display_line2

; Sends the byte stored in R0 to the display
send_byte:
	mov R16,R0 ; Send the upper nibble of R0 first.
	swap R16
	cbr R16,0xF0
	in R17,PINC ; Place the upper nibble of PORTC into R17.
	cbr R17,0x0F
	and R16,R17 ; Combine R17 upper nibble and R16 lower nibble.
	out PORTC,R16
	rcall LCD_strobe
	rcall delay_100us
	
	mov R16,R0 ; Then send the lower nibble of R0
	cbr R16,0xF0
	in R17,PINC ; Place the upper nibble of PORTC into R17.
	cbr R17,0x0F
	and R16,R17 ; Combine R17 upper nibble and R16 lower nibble.
	out PORTC,R16
	rcall LCD_strobe
	rcall delay_100us
	ret

LCD_strobe:
	; TODO: This
	ret

delay_100us:
	; TODO: This
	ret
delay_5ms:
	; TODO: This, probably use the delay_100us ~50 times
	ret
