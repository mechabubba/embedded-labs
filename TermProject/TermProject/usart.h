/**
 * # HI
 * usart related funcs stolen from uiowa s2024 ece3360 lab6 code.
 */
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#define BAUD_RATE 9600   // Baud rate. The usart_init routine uses this.

// Variables and #define for ring RX ring buffer.

#define RX_BUFFER_SIZE 64

unsigned char uart_buffer_empty(void);
void usart_prints(const char *ptr);
void usart_printf(const char *ptr);
void usart_init(void);
void usart_putc(const char c);
unsigned char usart_getc(void);
void usart_clear(void);

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
