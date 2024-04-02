/*
 * Lab05.c
 *
 * Created: 3/19/2024 11:05:17 AM
 * Author : Steven V, Jared M
 */ 

//Note: At 16 MHz, there are 16 clock cycles per microsecond.

#include <avr/io.h>
#include <avr/power.h>
#include <util/delay_basic.h>
#include <avr/sfr_defs.h>
#include <avr/interrupt.h>
#include "lcd.h"

#ifdef F_CPU
#define F_CPU 8000000UL //16 MHz clock speed assigned for the ATmega328P, divided by 2 for the scaling.
#endif

//Function Prototypes:
void checkRPG(void);
void updateLCD(void);

//Global constants (Program Memory):
const char okText[] PROGMEM = "Fan OK";
const char warnText[] PROGMEM = "Low RPM";
const char stallText[] PROGMEM = "Fan Stalled";

//Global variables (SRAM):
uint8_t compare = 50; //Compare value, equal to PWM percentage.
uint16_t tachometer = 0;
bool displayState = 0; //State value (0-1), state 0 is status mode and state 1 is duty cycle mode (for LCD).
char* buffer;

ISR(INT1_vect) { //External Interrupt 1 (Tachometer).
	TCCR1B &= ~0x03; //Turn off the clock.
	//uint8_t sreg = SREG;
	tachometer = TCNT1; //Collect time period.
	TCNT1 = 0x0000; //Reset timer.
	TCCR1B |= 0x03; //Turn clock back on.
	
}
//We can imagine that the fan RPM can range anywhere from 60 to 6000 RPM, where <60 RPM would likely be a stall condition.
//This equates to 1 to 100 Hz (Revolutions per second). At 8 MHz, this means 4 million half-cycles can occur in the worst case.
//If we divide Timer1 by 64, that gets us to a worst case of 62500 half-cycles, which can just barely fit within the 16-bit counter space of 65535.
//We can then assume that if the 16-bit counter hits its max value, a stall condition has been met.  
//The value out of the 16-bit timer (global variable tachometer) is thus in units of 8us / half-cycle.

ISR(TIMER0_OVF_vect) { //Timer0 overflow interrupt.
	OCR0B = compare; //Assign new PWM compare value.
}

int main(void) {
	clock_prescale_set(clock_div_2); //Scale down system clock to 8 MHz.
	
	//Data direction:
	DDRD |= (1 << PORTD5); // pin d5 (pwm) is output
	DDRC = 0x0F;           // first four port c pins are output
	DDRB |= (1 << PORTB3); // pin b3 (lcd e) is output
	DDRB |= (1 << PORTB5); // pin b5 (lcd rs) is output
	
	//Timer0 setup:
	TCCR0B |= (1 << WGM02); //Set timer0 to mode 5.
	TCCR0A &= ~(1 << WGM01);
	TCCR0A |= (1 << WGM00);
	TCCR0A |= (1 << COM0B1); //Set the OC0B flag to be used for comparing.
	TCCR0A &= ~(1 << COM0B0);
	TIMSK0 |= (1 << TOIE0); //Enable the overflow interrupt for timer0.
	OCR0A = 200u; //Set the TOP value to 200.
	OCR0B = 50u; //Set initial COMPARE value to 100.
	TCCR0B |= (1 << CS00); //Turn on the clock with no pre-scaling.


	lcd_init(LCD_DISP_ON); //Initialize the LCD.
	lcd_clrscr();
	
	uint8_t refresh = 0;
	sei(); //Enable interrupts.
    while (1) {
		checkRPG();
		if (refresh == 0) updateLCD(); //LCD is only updated every 256 cycles to increase visibility.
		refresh++;
    }
}

/* 20 KHz loop with resolution of <=0.75%
 * 1 / 0.0075 = 133.3 -> TOP > 134
 * TOP = 200 -> (16 MHz / 2) / 2*200 = 20 KHz <- Scale system clock down by 2(?).
 * Resolution = 1/200 = 0.5%
 */

void checkRPG(void) {
	if (bit_is_set(PORTB,PORTB1)) compare++;
	if (bit_is_set(PORTB,PORTB0)) compare--;
	if (compare > 200) compare = 200;
	if (compare < 0) compare = 0;
}

void updateLCD(void) {
	if (bit_is_clear(PORTD,PORTD2)) displayState = displayState ? 0 : 1; //Ternary operator to toggle the state variable.
	lcd_clrscr();
	lcd_home();
	lcd_puts(itoa(compare, buffer, 10)); //Display RPM.
	lcd_gotoxy(0,1);
	if (displayState) {
		lcd_puts(itoa(compare, buffer, 10)); //State 1 - display duty cycle.
	} else {
		//State 0 - display fan status.
	}
}
