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

#define LCD_RS_PORT PORTB
#define LCD_RS_PIN PINB5
#define LCD_E_PORT PORTB
#define LCD_E_PIN PINB3

#ifdef F_CPU
#define F_CPU 16000000UL //16 MHz clock speed assigned for the ATmega328P.
#endif

//Function Prototypes:
void checkRPG(void);
void showDutyCycle(void);
//void lcd_sendText(char text[], uint8_t length);
//void lcd_sendCommand(char command);
//void lcd_writeByte(const char b);
//void lcd_strobe(void);
//void lcd_initialize(void);

//Global variables (SRAM):
uint8_t compare = 50;

ISR(TIMER0_OVF_vect) { //Timer0 overflow interrupt.
	OCR0B = compare; //Assign new PWM compare value.
}

int main(void) {
	clock_prescale_set(clock_div_2); //Scale down system clock to 8 MHz.
	DDRD |= (1 << PORTD5);
	DDRC = 0x0F;
	DDRB |= (1 << PORTB3);
	DDRB |= (1 << PORTB5);
	
	TCCR0B |= (1 << WGM02); //Set timer0 to mode 5.
	TCCR0A &= ~(1 << WGM01);
	TCCR0A |= (1 << WGM00);
	
	TCCR0A |= (1 << COM0B1); //Set the OC0B flag to be used for comparing.
	TCCR0A |= (1 << COM0B0);
	
	TIMSK0 |= (1 << TOIE0); //Enable the overflow interrupt for timer0.
	OCR0A = 200u; //Set the TOP value to 200.
	OCR0B = 50u; //Set initial COMPARE value to 100.
	TCCR0B |= (1 << CS00); //Turn on the clock with no pre-scaling.
	
	//lcd_initialize(); //Initialize the LCD.
	lcd_init(LCD_DISP_ON);
	
	sei(); //Enable interrupts.
    while (1) {
		checkRPG();
		showDutyCycle();
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

void showDutyCycle(void) {
	//lcd_sendCommand(0x01); //Clear the display.
	//lcd_sendCommand(0xC0); //Move text pointer to 40
	lcd_clrscr();
	lcd_home();
	lcd_puts_p("Duty cycle = smth");
}
