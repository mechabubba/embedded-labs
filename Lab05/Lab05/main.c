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
#include <stdio.h>
#include "lcd.h"

#ifdef F_CPU
#define F_CPU 8000000UL //16 MHz clock speed assigned for the ATmega328P, divided by 2 for the scaling.
#endif

//Function Prototypes:
void checkRPG(void);
void checkButton(void);
void updateLCD(void);

//Global constants (Program Memory):
const char okText[] PROGMEM = "Fan OK";
const char warnText[] PROGMEM = "Low RPM";
const char stallText[] PROGMEM = "Fan Stalled";

//Global variables (SRAM):
uint8_t compare = 50; //Compare value, equal to PWM percentage.
uint16_t tachometer = 0;
uint8_t rpgState = 0x00; //Stores the last known state of the RPG pins for comparison.

uint8_t buttonState = 0; //Used for reading a toggle of the button.
uint8_t displayState = 0; //State value (0-1), state 0 is status mode and state 1 is duty cycle mode (for LCD).
char buffer[18]; //Largest string to be stored in buffer would be "Duty Cycle = XYZ%", which is 18 chars, including the null terminator.

uint16_t buzzerTimer = 0; // long "timer" for buzzer nonsense
uint8_t isBuzzerOn = 0;   // whether or not the buzzer is on (i love languages without official bools!)

////TODO: Fix tachometer code, probably just use floats for most of the calculations.
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
	DDRD |= (1 << PORTD7); // pin d7 (active buzzer) is output
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

	EICRA |= (1 << ISC11); //Turn on int1 on falling edge.
	TCCR1B |= 0x03; //Turn on tachometer clock.

	lcd_init(LCD_DISP_ON); //Initialize the LCD.
	lcd_clrscr();
	
	uint16_t refresh = 0;
	sei(); //Enable interrupts.
    while (1) {
		checkRPG();
		checkButton();
		
		if (refresh == 1024) {
			// lcd only updated after 1024 cycles. this lets us avoid the weird strobing that was happening earlier.
			updateLCD();
			refresh = 0;
		}
		refresh++;
		
		if (buzzerTimer == 65535) { // tweak to liking.
			isBuzzerOn = isBuzzerOn ? 0 : 1;
			if (isBuzzerOn) {
				PORTD |= (1 << PIND7);
			} else {
				PORTD &= ~(1 << PIND7);
			}
			buzzerTimer = 0;
		}
    }
}

/* 20 KHz loop with resolution of <=0.75%
 * 1 / 0.0075 = 133.3 -> TOP > 134
 * TOP = 200 -> (16 MHz / 2) / 2*200 = 20 KHz <- Scale system clock down by 2(?).
 * Resolution = 1/200 = 0.5%
 */

void checkRPG(void) {
	if ((PINB & 0x03) == 0x03) { //If pin state is on detent (b11 position), then previous state should be either 01 or 10.
		if (rpgState == 0x01) compare--; //If previous state was b01, then we moved counterclockwise.
		else if (rpgState == 0x02) compare++; //If previous state was b10, then we moved clockwise.
	}
	if (compare == 255) compare = 0; //Underflow catch.
	else if (compare > 200) compare = 200; //Overflow catch.
	rpgState = PINB & 0x03; //Store the current RPG pins for the next cycle.
}

void checkButton(void) {
	uint8_t avg = 127; // pretend this is a signed int...
	for (uint8_t i = 0; i < 11; i++) {
		if (bit_is_clear(PIND, PIND2)) avg++;
		else avg--;
	}
	buttonState <<= 1;
	buttonState &= 0x02;
	if (avg > 127) buttonState |= 0x01;
	if (buttonState == 0x01) displayState = displayState ? 0 : 1; //Ternary operator to toggle the state variable.
}

void updateLCD(void) {
	float rpm = 7500000.0f / (2.0f * (float)tachometer); //Probably a bad idea, but should convert the tachometer reading into a RPM float value.
	
	lcd_clrscr();
	lcd_home();
	sprintf(buffer,"RPM = %u",tachometer);
	lcd_puts(buffer); //Display RPM.
	
	lcd_gotoxy(0,1);
	if (displayState) { //State 1 - display duty cycle.
		sprintf(buffer,"%u%% Duty Cycle", compare >> 1); //shift compare left by 1 as it is 0.5% unit steps.
		lcd_puts(buffer);
	} else { //State 0 - display fan status.
		// temp code.
		sprintf(buffer, "%.2f", rpm);
		lcd_puts(buffer);
		
		// real code here...
		if (rpm < 60) { //Stall condition.
			lcd_puts(stallText);
		} else if (rpm < 2400) { //Low RPM warning.
			lcd_puts(warnText);
		} else { //Fan is operating normally.
			lcd_puts(okText);
		}
	}
}
