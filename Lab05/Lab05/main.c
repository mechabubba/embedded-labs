/*
 * Lab05.c
 *
 * Created: 3/19/2024 11:05:17 AM
 * Author : Steven V, Jared M
 */ 

//Note: At 16 MHz, there are 16 clock cycles per microsecond.

#include <avr/io.h>
#include <util/delay_basic.h>

#ifdef F_CPU
#define F_CPU 16000000UL //16 MHz clock speed assigned for the ATmega328P.
#endif

//Function Prototypes:
void lcd_sendText(char text[], int length);
void lcd_sendCommand(char command);
void lcd_writeByte(const char b);
void lcd_strobe(void);
void lcd_initialize(void);

int main(void)
{
	lcd_initialize();
    while (1) 
    {
    }
}

void lcd_sendText(char text[], int length) {
	PORTB |= (1 << 5); //Set RS to data mode.
	for (int i = 0; i < length; i++) {
		lcd_writeByte(text[i]); //Send each character in text[] one at a time.
	}
}

void lcd_sendCommand(char command) {
	PORTB &= ~(1 << 5); //Set RS to command mode.
	lcd_writeByte(command);
	if (command == 0x01 || command == 0x02 || command == 0x03) { //If command is CLEAR_DISPLAY or CURSOR_HOME, then:
		_delay_loop_2(6800); //Wait 6800 iterations = 27200 clock cycles = 1700us.
	}
}

void lcd_writeByte(const char b) {
	char tmp = b & 0xF0; //Set upper nibble of tmp to upper nibble of b.
	tmp >>= 4; //Swap upper and lower nibble of tmp. I don't know if we have access to a method like SWAP, so this might be slow.
	char pcUpper = PORTC & 0xF0; //Store the upper nibble of PORTC.
	PORTC = pcUpper | tmp; //Write upper nibble of b to the output.
	lcd_strobe(); //Strobe the enable line, then run the 0.5us delay.
	_delay_loop_1(3); //3 iterations >= 9 clock cycles > 0.5us.
	
	tmp = b * 0x0F; //Set lower nibble of tmp to lower nibble of b.
	pcUpper = PORTC & 0xF0; //Store the upper nibble of PORTC again.
	PORTC = pcUpper | tmp; //Write lower bits of b to the output.
	lcd_strobe();
	_delay_loop_1(214); //214 iterations >= 642 clock cycles > 40us (For standard commands and text entry).
}

void lcd_strobe(void) {
	PORTB &= ~(1 << 3); //Clear PB3.
	PORTB |= (1 << 3); //Set PB3 (Enable).
	_delay_loop_1(2); //2 iteration delay loop guarantees at least 6 clock cycles pass, which is over 230ns.
	PORTB &= ~(1 << 3); //Clear PB3 (Enable).
}

void lcd_initialize(void) {
	for (int i = 0; i < 6; i++) {
		_delay_loop_2(0);
	}
	_delay_loop_2(6784); //Should total to approximately 0.1 seconds
	
	lcd_sendCommand(0x03); //Set LCD to 8-bit mode.
	_delay_loop_2(20000); //Wait 5ms.
	lcd_sendCommand(0x03); //Set LCD to 8-bit mode.
	_delay_loop_2(800); //Wait 200us.
	lcd_sendCommand(0x03); //Set LCD to 8-bit mode.
	_delay_loop_2(800); //Wait 200us.
	lcd_sendCommand(0x02); //Set LCD to 4-bit mode.
	_delay_loop_2(800); //Wait 200us.
	
	lcd_sendCommand(0x28); //Set LCD to 4-bit, 2-line, 5x7-display mode.
	lcd_sendCommand(0x06); //Set LCD to increment, display shift off.
	lcd_sendCommand(0x0C); //Set LCD display on.
}

