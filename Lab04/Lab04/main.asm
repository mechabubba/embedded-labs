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
; (R28,R29) is the Y register, which temporarily stores the state of the Z register for returning to.
; R0 is the register used when the send_byte subroutine is called.
; R27 is the counter used to track what word is being displayed.
; R26 is the register that stores the command for the send_command subroutine.
; R26 is the button state register - bit 0 is the current button state, bit 1 is the previous button state.

; R16, R17 are temporary registers and will not preserve their value across a subroutine call (Used by send_byte, delay_100us, etc.).

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
	ldi R30,LOW(2*words) ; Set the Z-register to the "words" label.
	ldi R31,2*HIGH(words)

	; Initializing the LCD Display
	rcall delay_100ms ; Wait 0.1 seconds before sending commands to the LCD.

	cbi PORTB,5 ; Setting the LCD to command mode.

	in R16,PINC
	cbr R16,0x0F
	andi R16,0x03
	out PORTC,R16 ; Send 8-bit command to LCD
	rcall LCD_strobe
	rcall delay_5ms ; Wait 5 ms.

	in R16,PINC
	cbr R16,0x0F
	andi R16,0x03
	out PORTC,R16 ; Send 8-bit command to LCD
	rcall LCD_strobe
	rcall delay_100us ; Wait 200 us.
	rcall delay_100us

	in R16,PINC
	cbr R16,0x0F
	andi R16,0x03
	out PORTC,R16 ; Send 8-bit command to LCD
	rcall LCD_strobe
	rcall delay_100us ; Wait 200 us.
	rcall delay_100us

	in R16,PINC
	cbr R16,0x0F
	andi R16,0x02
	out PORTC,R16 ; Send 4-bit command to LCD
	rcall LCD_strobe
	rcall delay_100us ; Wait 200 us.
	rcall delay_100us

	cbi PORTC,5 ; Once initialization is complete, turn LED back on.

	rjmp update_text ; Write the initial text to the display.

loop:
	; Button logic:
	lsl R25 ; Update past button state (bit 2)
	andi R25,0x02 ; Don't let the other 6 bits get set
	sbis PIND,2 ; If button is pressed (PD2 is pulled low), set bit 0 of R25 to 1. Otherwise it will be 0 due to the logical shift left.
	sbr R25,0 ; Set bit 0 to 1.

	cpi R25,0x01 ; Check if button was just pressed (button is pressed now but was not pressed on the last loop).
	breq update_text ; If so then update the text accordingly.
	rcall delay_5ms ; Otherwise wait 5 milliseconds before checking again (acts as a debounce).
	rjmp loop

; To make the first line display what was previously on the second line, we will on button press do:
; Clear LCD; Set LCD pointer to 0x00 (Line 1); run display_line; run cycle_words; Set LCD pointer to 0x40 (Line 2); run display_line; move Z back to begining of line.

; Runs on button press, Not a subroutine (accessed through branch statement in loop)
update_text:
	ldi R26,0x01 ; Send the Clear Display command to LCD.
	rcall send_command

	ldi R26,0x80 ; Set the DDRAM pointer to 0x00 (Start of line 1)
	rcall send_command

	rcall display_line ; Display line 1
	rcall cycle_words

	ldi R26,0xC0 ; Set the DDRAM pointer to 0x40 (Start of line 2)
	rcall send_command

	mov R28,R30 ; Store start of second line to return to
	mov R29,R31
	rcall display_line ; Display line 2
	mov R30,R28 ; Set Z to what it was before now, so we can output this again next time.
	mov R31,R29
	rjmp loop

cycle_words:
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
	

; Sends the text pointed to by the Z-register to the display.
display_line:
	sbi PORTB,5 ; Ensure that we are in text mode, not command mode.
	lpm R0,Z+ ; Load byte into R0.
	tst R0 ; Does R0 = 0x00 (Null character)?
	breq _end ; If so then exit the loop.
	rcall send_byte
	rjmp display_line ; Loop back to start
_end:
	ret ; Exit display_line

; Sends a command stored in R26 to the display.
send_command:
	cbi PORTB,5 ; Set the LCD to command mode
	mov R0,R26
	rcall send_byte
	cpi R26,0x04 ; If command is not Clear Display or Cursor Home, end here.
	brsh _end
	rcall delay_5ms ; Otherwise wait 5 ms to exceed the 1.64 ms safety margin.
	ret

; Sends the byte stored in R0 to the display, either for commands or for text.
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

; Sets the Enable line from low to high, waits 250 ns, then clears the Enable line back to low.
LCD_strobe:
	cbi PORTB,3 ; Set Enable to low.
	sbi PORTB,3 ; Set Enable to high.
	nop ; 4 NOP instructions so at least 230 nanoseconds pass (about 250 ns should pass).
	nop
	nop
	nop
	cbi PORTB,3 ; Set Enable back to low
	ret

; Note: at 16 MHz, 1 clock cycle is 0.0625 microseconds (0.0625 us = 62.5 ns)
; Therefore, 100 us is 1600 clock cycles.

; Approximately 100 us delay (1606 cycles).
delay_100us:
	ldi r16,255 ; 5*255 + 5*65 = 1600 cycles
	_d1:
		nop
		nop
		dec r16
		brne _d1
	ldi r16,65
	_d2:
		nop
		nop
		dec r16
		brne _d2
	ret

; Approximately 5 ms delay.
delay_5ms:
	ldi r17,50
	_d3:
		rcall delay_100us
		dec r17
		brne _d3
	ret

; Approximately 100 ms delay.
delay_100ms:
	ldi r18,20
	_d4:
		rcall delay_5ms
		dec r18
		brne _d4
	ret
