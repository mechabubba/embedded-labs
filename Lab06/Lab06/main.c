/*
 * Lab06/main.c
 *
 * Created: 04/16/2024 02:49:00 PM
 * Author : Steven V, Jared M
 */ 

#ifndef F_CPU
#define F_CPU 16000000UL    // 16 MHz clock speed.
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include "avr/sfr_defs.h"
#include <stdio.h>
#include "i2cmaster.h"
#include <time.h>

void _delay_5ms(void);
void showMenu(void);

unsigned char uart_buffer_empty(void);
void usart_prints(const char *ptr);
void usart_printf(const char *ptr);
void usart_init(void);
void usart_putc(const char c);
unsigned char usart_getc(void);
void usart_clear(void);

void getPCTime(struct tm *rtc_date);
void getPCF8583Time(struct tm *rtc_date);
void setPCF8583Time(struct tm *rtc_date);

uint8_t to_bcd(uint8_t in);
uint8_t from_bcd(uint8_t in);

// debug methods.
#define COOL_DEBUG_FLAG // comment this out if you don't want 'em.
#ifdef COOL_DEBUG_FLAG
  #define _usart_putc(c)   usart_putc(c);
  #define _usart_prints(s) usart_prints(s);
#else
  #define _usart_putc(c)
  #define _usart_prints(s)
#endif

// cool globals.
struct tm rtc_date;
char* rtc_str;

// Store menu items in FLASH.
const char menu_text[] PROGMEM =
   "*************************************\r\n"
   "Press any of these keys;\r\n"
   "  M - Stop data acquisition and show menu.\r\n"
   "  R - Run and collect data.\r\n"
   "  T - Get time from PC and set RTC.\r\n"
   "  S - Print current time on the RTC.\r\n"
   "*************************************\r\n";

// using interrupts for evil here and doing absolutely everything inside this one.
ISR(TIMER1_COMPA_vect) {
	cli(); // disabling the other interrupts for a short time while in here.
	
	getPCF8583Time(&rtc_date);
	rtc_str = isotime(&rtc_date);
	usart_prints(rtc_str);
	usart_prints(", ");
	
	uint16_t val = ADC; //Since we are in an interrupt that pauses other interrupts, we can trust that this operation will remain atomic.
	//Therefore we should be able to just use the 16-bit ADC, rather than separately access the 8-bit ADCH and ADCL registers.
	char buff[16];
	usart_prints(itoa(val, buff, 10));
	usart_prints("\r\n");

	sei(); // get crackin'
}

int main (void)
{  
   DDRD |= (1 << PORTD7);  // buzzer out.
   DDRC &= ~(1 << PORTC0); // analog signal in. (is this necessary?)
   
   // ADC stuff...
   ADMUX = 0x40;  // REFS = 01b, ADLAR = 0b,  MUX = 0000b.
   ADCSRB = 0x00; // ACME = 0b,  ADTS = 0000b (Free running mode? We are not told what to do here.)
   ADCSRA = 0xC0; // ADEN = 1b,  ADSC = 1b,   All else = 0. 
   //Will trigger the first conversion, and because of free-running mode the uC should continuously try and perform conversions from that point on.
   
   //Timer1 setup - We have to print the value once per second so we need a way to know that one second has passed (without using the INT0 pin on the RTC?)
   //The easiest way I could think of is Timer1 on CTC mode, where the clock is divided by 256 and reset is triggered at value 62500.
   TCCR1A = 0x00;
   OCR1A = 62500; //Assign TOP value to 62500 to get a 1 Hz interrupt signal.
   TCCR1B |= (1 << WGM12) | (1 << CS12); // CTC mode (OCR1A top value), Divide clock by 256 and enable.

   usart_init();           // Initialize the USART
   i2c_init();             // Initialize I2C system.
   sei();                  // Enable interrupts.
   showMenu();

   char c;
   while(1) {
      if (!uart_buffer_empty()){  // If there are input characters.
         c = usart_getc();
         
		 // beep.
		 if((TIMSK1 & (1 << OCIE1A)) && (c != 'm' && c != 'M')) {
			 _usart_prints("audible beep. ");
			 PORTD |= (1 << PIND7);
			 _delay_ms(100);
			 PORTD &= ~(1 << PIND7);
		 } else {
            switch(c) { //Switches based on what character the terminal has sent to the uC.
               case 't':
               case 'T':
                  usart_prints("Getting time from PC. Run isotime.vbs.\r\n");
                  getPCTime(&rtc_date);         // Gets PC time in format:
                  setPCF8583Time(&rtc_date);    // Set the RTC.
                  break;
      
	           case 'r':
               case 'R':
                  usart_prints("Starting log...\r\n");
			      TIMSK1 |= (1 << OCIE1A); //Enable the timer1 interrupt.
				  //I was an idiot here, it would make far more sense to pause Timer1's clock rather than pause its interrupt,
				  //but since it is currently working I don't want to risk breaking anything.
			      break;
			   
			   case 'm':
			   case 'M':
			      if ((TIMSK1 & (1 << OCIE1A)) == 0) break; //The interrupt is not enabled in the first place.
			      TIMSK1 &= ~(1 << OCIE1A); //Disable the timer1 interrupt.
			      usart_prints("Ending log.\r\n");
				  usart_printf(menu_text);
			      break;
			   
			   case 's':
			   case 'S': //Just print the current time?
			      getPCF8583Time(&rtc_date);
			      rtc_str = isotime(&rtc_date);
			      usart_prints(rtc_str);
			      usart_prints("\r\n");
			      break;

			   default:
                  usart_prints("Beep\r\n");
                  usart_clear();
			}
		 }
      }
   }
}

void showMenu(void) {  
   usart_printf(menu_text);
}

////////////////////////////////////////////////////////////////
//
// Get time from PC via serial port. The format the PC send is a
// 
//      yyyy-MM-dd hh:mm:ss  2024-03-28 22:51:41vBCrLf
// 
// where vbCrLf is \r\n or 0x0D 0x0A. Months are 1..12.
//
// The function blocks.
//

#define GET_DATE_VAL(val, size)      \
    char (val)[(size + 1)];          \
	for (int i = 0; i < size; i++) { \
		(val)[i] = usart_getc();     \
		_usart_putc((val)[i]);       \
	}                                \
	usart_getc(); // dump following character.

void getPCTime(struct tm *rtc_date) // 2024-03-28 22:51:41
{
   GET_DATE_VAL(year, 4);
   GET_DATE_VAL(month, 2);
   GET_DATE_VAL(day, 2);
   GET_DATE_VAL(hour, 2);
   GET_DATE_VAL(minute, 2);
   GET_DATE_VAL(second, 2);

   usart_getc(); // '\n'
   
   rtc_date->tm_year = atoi(year) - 1900; // this is a measure of years since 1900
   rtc_date->tm_mon  = atoi(month);
   rtc_date->tm_mday = atoi(day);
   rtc_date->tm_hour = atoi(hour);
   rtc_date->tm_min  = atoi(minute);
   rtc_date->tm_sec  = atoi(second);
}

////////////////////////////////////////////////////////////////
//
//  Sets the PCF8583 RTC using contents of rtc_date.
//
void setPCF8583Time(struct tm *rtc_date)
{
   i2c_start_wait(0xA0 + I2C_WRITE);
   i2c_write(0x00);
   i2c_write(0x80);
   i2c_stop();
   _usart_prints("[setPCF8583Time] paused RTC... ");
   
   i2c_start_wait(0xA0 + I2C_WRITE); 
   i2c_write(0x02);
   i2c_write(to_bcd(rtc_date->tm_sec)); //Write the second counter. (02h)
   i2c_write(to_bcd(rtc_date->tm_min)); //Write the minute counter. (03h)
   i2c_write(to_bcd(rtc_date->tm_hour) & 0x3F); //Write the hour counter. (04h)
   i2c_write(((rtc_date->tm_year & 0x03) << 6) + to_bcd(rtc_date->tm_yday)); //Write the year/day counter. (05h)
   i2c_write(to_bcd(rtc_date->tm_mon)); //Write the month counter (weekdays are unknown). (06h)
   i2c_stop();
   _usart_prints("wrote RTC... ");
   
   i2c_start_wait(0xA0 + I2C_WRITE); //Resume the RTC clock.
   i2c_write(0x00);
   i2c_write(0x00);
   i2c_stop();
   _usart_prints("unpaused rtc.\r\n");
}


////////////////////////////////////////////////////////////////
//
//  Get PCF8583 RTC time and fill rtc_date.
//
void getPCF8583Time(struct tm *rtc_date)
{
   i2c_start(0xA0 + I2C_WRITE); //Tell it to start reading from the seconds address.
   i2c_write(0x02);
   i2c_stop();
   
   i2c_start(0xA0 + I2C_READ);
   rtc_date->tm_sec = from_bcd(i2c_readAck());
   rtc_date->tm_min = from_bcd(i2c_readAck());
   rtc_date->tm_hour = from_bcd(i2c_readAck());
   rtc_date->tm_yday = from_bcd(i2c_readAck() & 0x3F); //Don't include the year bits
   rtc_date->tm_mon = from_bcd(i2c_readNak());
}

// Helper Function

void _delay_5ms(void)  // This assumes an 8-MHz clock.
{
   _delay_loop_2((uint16_t)9000);
   _delay_loop_2((uint16_t)1000);
}


////////////////////////////// USART Related Are Below ////////////////////////////

#define BAUD_RATE 9600   // Baud rate. The usart_init routine uses this.

// Variables and #define for ring RX ring buffer.

#define RX_BUFFER_SIZE 64
unsigned char rx_buffer[RX_BUFFER_SIZE];
volatile unsigned char rx_buffer_head;
volatile unsigned char rx_buffer_tail;

ISR(USART_RX_vect)
{
   // UART receive interrupt handler.
   // To do: check and warn if buffer overflows.
   
   char c = UDR0;
   rx_buffer[rx_buffer_head] = c;
   if (rx_buffer_head == RX_BUFFER_SIZE - 1)
   rx_buffer_head = 0;
   else
   rx_buffer_head++;
}

void usart_clear(void)
{
   rx_buffer_tail = rx_buffer_head;
}

void usart_init(void)
{
   // Configures the USART for serial 8N1 with
   // the Baud rate controlled by a #define.

   unsigned short s;
   
   // Set Baud rate, controlled with #define above.
   
   s = (double)F_CPU / (BAUD_RATE*16.0) - 1.0;
   UBRR0H = (s & 0xFF00);
   UBRR0L = (s & 0x00FF);

   // Receive complete interrupt enable: RXCIE0
   // Receiver & Transmitter enable: RXEN0,TXEN0

   UCSR0B = (1<<RXCIE0)|(1<<RXEN0)|(1<<TXEN0);

   // Along with UCSZ02 bit in UCSR0B, set 8 bits
   
   UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
   
   DDRD |= (1<< 1);         // PD0 is output (TX)
   DDRD &= ~(1<< 0);        // PD1 is input (Rx)
   
   // Empty buffers
   
   rx_buffer_head = 0;
   rx_buffer_tail = 0;
}

void usart_printf(const char *ptr){

   // Send NULL-terminated data from FLASH.
   // Uses polling (and it blocks).

   char c;

   while(pgm_read_byte_near(ptr)) {
      c = pgm_read_byte_near(ptr++);
      usart_putc(c);
   }
}

void usart_putc(const char c){

   // Send "c" via the USART.  Uses poling
   // (and it blocks). Wait for UDRE0 to become
   // set (=1), which indicates the UDR0 is empty
   // and can accept the next character.

   while (!(UCSR0A & (1<<UDRE0)))
   ;
   UDR0 = c;
}

void usart_prints(const char *ptr){
   
   // Send NULL-terminated data from SRAM.
   // Uses polling (and it blocks).

   while(*ptr) {
      while (!( UCSR0A & (1<<UDRE0)))
      ;
      UDR0 = *(ptr++);
   }
}

unsigned char usart_getc(void)
{

   // Get char from the receiver buffer.  This
   // function blocks until a character arrives.
   
   unsigned char c;
   
   // Wait for a character in the buffer.

   while (rx_buffer_tail == rx_buffer_head)
         ;
   
   c = rx_buffer[rx_buffer_tail];
   if (rx_buffer_tail == RX_BUFFER_SIZE-1)
   rx_buffer_tail = 0;
   else
   rx_buffer_tail++;
   return c;
}

unsigned char uart_buffer_empty(void)
{
   // Returns TRUE if receive buffer is empty.
  
   return (rx_buffer_tail == rx_buffer_head);
}


////////////////////////////// IC2 Related Are Below ////////////////////////////

#include <inttypes.h>
#include <compat/twi.h>
#include "i2cmaster.h" //i2cmaster.h is a local header, not a library.

// define CPU frequency in Here here if not defined in Makefile  or above 

#ifndef F_CPU
#define F_CPU 4000000UL
#endif

/* I2C clock in Hz */
#define SCL_CLOCK  100000L


/*************************************************************************
 Initialization of the I2C bus interface. Need to be called only once
*************************************************************************/
void i2c_init(void)
{
  /* initialize TWI clock: 100 kHz clock, TWPS = 0 => prescaler = 1 */
  
  TWSR = 0;                         /* no prescaler */
  TWBR = ((F_CPU/SCL_CLOCK)-16)/2;  /* must be > 10 for stable operation */

}/* i2c_init */


/*************************************************************************	
  Issues a start condition and sends address and transfer direction.
  return 0 = device accessible, 1= failed to access device
*************************************************************************/
unsigned char i2c_start(unsigned char address)
{
    uint8_t   twst;

	// send START condition
	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);

	_usart_prints("Sending START...\r\n");

	// wait until transmission completed
	while(!(TWCR & (1<<TWINT)));
	
	_usart_prints("START sent!\r\n");

	// check value of TWI Status Register. Mask prescaler bits.
	twst = TW_STATUS & 0xF8;
	if ( (twst != TW_START) && (twst != TW_REP_START)) return 1;

	// send device address
	TWDR = address;
	TWCR = (1<<TWINT) | (1<<TWEN);

	// wail until transmission completed and ACK/NACK has been received
	while(!(TWCR & (1<<TWINT)));

	// check value of TWI Status Register. Mask prescaler bits.
	twst = TW_STATUS & 0xF8;
	if ( (twst != TW_MT_SLA_ACK) && (twst != TW_MR_SLA_ACK) ) return 1;

	return 0;

}/* i2c_start */


/*************************************************************************
 Issues a start condition and sends address and transfer direction.
 If device is busy, use ack polling to wait until device is ready
 
 Input:   address and transfer direction of I2C device
*************************************************************************/
void i2c_start_wait(unsigned char address)
{
    uint8_t   twst;


    while ( 1 )
    {
	    // send START condition
	    TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN);
    
    	// wait until transmission completed
    	while(!(TWCR & (1<<TWINT)));
    
    	// check value of TWI Status Register. Mask prescaler bits.
    	twst = TW_STATUS & 0xF8;
    	if ( (twst != TW_START) && (twst != TW_REP_START)) continue;
    
    	// send device address
    	TWDR = address;
    	TWCR = (1<<TWINT) | (1<<TWEN);
    
    	// wail until transmission completed
    	while(!(TWCR & (1<<TWINT)));
    
    	// check value of TWI Status Register. Mask prescaler bits.
    	twst = TW_STATUS & 0xF8;
    	if ( (twst == TW_MT_SLA_NACK )||(twst ==TW_MR_DATA_NACK) ) 
    	{    	    
    	    /* device busy, send stop condition to terminate write operation */
	        TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	        
	        // wait until stop condition is executed and bus released
	        while(TWCR & (1<<TWSTO));
	        
    	    continue;
    	}
    	//if( twst != TW_MT_SLA_ACK) return 1;
    	break;
     }

}/* i2c_start_wait */


/*************************************************************************
 Issues a repeated start condition and sends address and transfer direction 

 Input:   address and transfer direction of I2C device
 
 Return:  0 device accessible
          1 failed to access device
*************************************************************************/
unsigned char i2c_rep_start(unsigned char address)
{
    return i2c_start( address );

}/* i2c_rep_start */


/*************************************************************************
 Terminates the data transfer and releases the I2C bus
*************************************************************************/
void i2c_stop(void)
{
    /* send stop condition */
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	
	// wait until stop condition is executed and bus released
	while(TWCR & (1<<TWSTO));

}/* i2c_stop */


/*************************************************************************
  Send one byte to I2C device
  
  Input:    byte to be transfered
  Return:   0 write successful 
            1 write failed
*************************************************************************/
unsigned char i2c_write( unsigned char data )
{	
    uint8_t   twst;
    
	// send data to the previously addressed device
	TWDR = data;
	TWCR = (1<<TWINT) | (1<<TWEN);

	// wait until transmission completed
	while(!(TWCR & (1<<TWINT)));

	// check value of TWI Status Register. Mask prescaler bits
	twst = TW_STATUS & 0xF8;
	if( twst != TW_MT_DATA_ACK) return 1;
	return 0;

}/* i2c_write */


/*************************************************************************
 Read one byte from the I2C device, request more data from device 
 
 Return:  byte read from I2C device
*************************************************************************/
unsigned char i2c_readAck(void)
{
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);
	while(!(TWCR & (1<<TWINT)));    

    return TWDR;

}/* i2c_readAck */


/*************************************************************************
 Read one byte from the I2C device, read is followed by a stop condition 
 
 Return:  byte read from I2C device
*************************************************************************/
unsigned char i2c_readNak(void)
{
	TWCR = (1<<TWINT) | (1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
	
    return TWDR;

}/* i2c_readNak */

uint8_t to_bcd(uint8_t in) {
	return (in % 10) | ((in / 10) << 4);
}

uint8_t from_bcd(uint8_t in) {
	return (in & 0x0F) + 10 * (in >> 4);
}
