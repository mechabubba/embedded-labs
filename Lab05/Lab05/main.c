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
uint8_t rpgState = 0x00; //Stores the last known state of the RPG pins for comparison.
uint8_t displayState = 0; //State value (0-1), state 0 is status mode and state 1 is duty cycle mode (for LCD).
char buffer[18]; //Largest string to be stored in buffer would be "Duty Cycle = XYZ%", which is 18 chars, including the null terminator.

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
	DDRB &= ~(1 << PINB0); // pin b0 (RPG) is input
	DDRB &= ~(1 << PINB1); // pin b1 (RPG) is input
	
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
	if (bit_is_set(PINB,PINB1) ^ (rpgState & 0x01)) compare++; //Clockwise check - does PB1 not equal PB0 previous state?
	if (bit_is_set(PINB,PINB0) ^ (rpgState >> 1 & 0x01)) compare--; //Counterclockwise check - does PB0 not equal PB1 previous state?
	//If both bits have changed, we cannot know which way the RPG has rotated so they should cancel out.
	if (compare > 200) compare = 200;
	if (compare < 0) compare = 0; //Compare is an unsigned 8-bit, so this comparison may be non-functional.
	rpgState = PINB & 0x02; //Store the current RPG pins for the next cycle.
}

void updateLCD(void) {
	if (bit_is_clear(PIND,PIND2)) displayState = displayState ? 0 : 1; //Ternary operator to toggle the state variable.
	float rpm = 7500000/(2*tachometer); //Probably a bad idea, but should convert the tachometer reading into a RPM float value.
	
	lcd_clrscr();
	lcd_home();
	sprintf(buffer,"RPM = %.0f",rpm);
	lcd_puts(buffer); //Display RPM.
	lcd_gotoxy(0,1);
	if (displayState) {
		sprintf(buffer,"Duty cycle = %u%%",compare);
		lcd_puts(buffer); //State 1 - display duty cycle.
	} else {
		if (rpm < 60) { //Stall condition.
			lcd_puts(stallText);
		} else if (rpm < 1000) { //Warn about possible stall.
			lcd_puts(warnText);
		} else { //Fan is operating normally.
			lcd_puts(okText);
		}
		//State 0 - display fan status.
	}
}
