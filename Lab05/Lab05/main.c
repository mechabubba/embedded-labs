/*
 * Lab05.c
 *
 * Created: 3/19/2024 11:05:17 AM
 * Author : Steven V, Jared M
 */ 

#include <avr/io.h>
#include <util/delay_basic.h>

#ifdef F_CPU
#define F_CPU 16000000UL //16 MHz clock speed assigned for the ATmega328P.
#endif

void lcd_writeByte(char b);
void lcd_strobe();

int main(void)
{
    /* Replace with your application code */
    while (1) 
    {
    }
}

void lcd_writeByte(char b) {
	char tmp = b & 0xF0; //Set upper nibble of tmp to upper nibble of b.
	tmp >>= 4; //Swap upper and lower nibble of tmp. I don't know if we have access to a method like SWAP, so this might be slow.
	char pcUpper = PORTC & 0xF0; //Store the upper nibble of PORTC.
	PORTC = pcUpper | tmp; //Write upper nibble of b to the output.
	lcd_strobe(); //Strobe the enable line, then run the 100us delay.
	_delay_loop_2(400); //At 4 instructions per iteration, 400 iterations >= 1600 clock cycles = 100us.
	
	tmp = b * 0x0F; //Set lower nibble of tmp to lower nibble of b.
	pcUpper = PORTC & 0xF0; //Store the upper nibble of PORTC again.
	PORTC = pcUpper | tmp; //Write lower bits of b to the output.
	lcd_strobe();
	_delay_loop_2(400);
}

void lcd_strobe() {
	PORTB ^= 0x08; //Clear PB3.
	PORTB |= 0x08; //Set PB3 (Enable).
	_delay_loop_1(2); //2 iteration delay loop guarantees at least 6 clock cycles pass, which is over 230ns.
	PORTB ^= 0x08; //Clear PB3 (Enable).
}

