/*
 * TermProject
 *
 * Created: 4/23/2024 1:58:54 PM
 * Author : Jared Mulder, Steven Vanni
 */ 

#ifndef F_CPU
#define F_CPU 16000000UL    // 16 MHz clock speed.
#endif

#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include "i2cmaster.h"
#include "usart.h"

/* Preprocessor stuff */
#define SENSOR_ADDR (0x38 << 1)
#define LED_ADDR    (0x74 << 1)
// We can only attach 1 LED display as the 2 displays (probably) share the same I2C address.
// If one of them is a KTD2502A/KTD2502B and the other is a KTD2502C/KTD2502D, then they would have different addresses.

//Global Variables:
uint8_t redVals[4];
uint8_t greenVals[4];
uint8_t blueVals[4];
uint8_t cIndex = 0; //The colors are stored in a circular buffer, and this tracks where we are in the buffer.

uint8_t buttonState = 0; //Weird state-control button system we have used before. 00 = not pressed, 01 = just pressed, 11 = held down, 10 = just unpressed.

//Function prototypes:
void writeColor(uint8_t red, uint8_t green, uint8_t blue);
void getColor(void);
void updateLED(void);
void printColor(uint8_t index);
uint8_t checkButton(void);

int main(void)
{
	/* Initialization */
	DDRC |= (1 << PINC0); // button
	
	usart_init();
	i2c_init();
	sei();
	usart_prints("Starting.\r\n");
	
	// The below code works 1/10th of the time and I have absolutely zero clue why.
	// Fuck.
	//_delay_ms(1000); // For the sensor???
	//char stat_sens = i2c_start(I2C_WRITE + SENSOR_ADDR);
	//if (stat_sens) {
	//	usart_prints("Failed to access sensor.\r\n");
	//} else {
	//	i2c_write(0x00); // 0x00 SYSM_CTRL
	//	i2c_write(0x01); // Turn on color sens function.
	//	i2c_stop();
	//}
	
	writeColor(255, 0, 255); // temp
	char stat_led = i2c_start(I2C_WRITE + LED_ADDR);
	if (stat_led) {
		usart_prints("Failed to access LEDs.\r\n");
	} else {
		i2c_write(0x02); // 0x02 CONTROL
		i2c_write(1 << 7); // Enable normal mode. 
		i2c_stop();
	}

	usart_prints("Everything should be initiated...\r\n");

	/* Main loop */
	char c; //Input character
    while (1) {
		//buttonState = (0x03 & (buttonState << 1)) | checkButton();
		if (!uart_buffer_empty()) {
			c = usart_getc();
			switch (c) {
			case 'r':
			case 'R': // Set red color.
				writeColor(255,0,0);
				updateLED();
				break;
			case 'g':
			case 'G': // Set green color.
				writeColor(0,255,0);
				updateLED();
				break;
			case 'c':
			case 'C': // Print colors in matrix.
				printColor(0);
				printColor(1);
				printColor(2);
				printColor(3);
				break;
			case 's':
			case 'S': // Get color from sensor, set it, and update the LED.
				getColor();
				updateLED();
				break;
			default: // Echoes chars.
				usart_prints("Echo; ");
				usart_putc(c);
				usart_prints("\r\n");
				usart_clear();
				break;
			}
		}

		if (buttonState == 0x01) {
			usart_prints("\tButton pushed.\r\n");
			getColor();
			updateLED();
		}
    }
}

//Writes a color to the color buffer and increments cIndex.
void writeColor(uint8_t red, uint8_t green, uint8_t blue) {
	redVals[cIndex] = red;
	greenVals[cIndex] = green;
	blueVals[cIndex] = blue;
	cIndex = (cIndex >= 4) ? 0 : cIndex + 1; //Increment cIndex unless it is at (or above) 7, then set it to 0.
	//cIndex will cycle forward through [0..7] before repeating.
}


//Complete function for reading color data in from the sensor and writing it into the buffer with the writeColor method.
void getColor(void) {
	uint8_t r = 0, g = 0, b = 0;
	_delay_ms(1000); // For the sensor???
	i2c_start(SENSOR_ADDR+I2C_WRITE);
	i2c_write(0x00);
	i2c_write(0x01); //Turn on the color sensor.
	i2c_stop();
	
	//Don't know if this is enough time to wait or not.
	i2c_start(SENSOR_ADDR+I2C_WRITE);
	i2c_write(0x1C); //Access the color buffer.
	i2c_start(SENSOR_ADDR+I2C_READ);
	i2c_readAck(); //Dump the lower 8 bits as we are only storing 8-bit color channels, not 16-bit.
	r = i2c_readAck();
	i2c_readAck();
	g = i2c_readAck();
	i2c_readAck();
	b = i2c_readNak();
	
	i2c_start(SENSOR_ADDR+I2C_WRITE);
	i2c_write(0x00);
	i2c_write(0x00); //Turn off the color sensor.
	i2c_stop();
	
	writeColor(r,g,b);
}

void updateLED(void) {
	i2c_start(LED_ADDR+I2C_WRITE);
	i2c_write(0x02);
	i2c_write(0x00); //Turn off the LEDs while we are changing the values.
	for (uint8_t i = 0; i < 4; i++) {
		i2c_write(redVals[i]);
		i2c_write(greenVals[i]);
		i2c_write(blueVals[i]);
	}
	i2c_stop();
	
	i2c_start(LED_ADDR+I2C_WRITE);
	i2c_write(0x02);
	i2c_write(0x80); //Turn the LED back on afterwards (requires a new send as we need to go back to the previous address.)
	i2c_stop();
}

uint8_t checkButton(void) {
	uint8_t acc = 127;
	for (uint8_t i = 0; i < 25; i++) {
		if ((PORTC >> PINC0) & 0x01) {
			acc++;
		} else {
			acc--;
		}
	}
	if (acc > 127) {
		usart_prints("push\r\n");
		return 1;
	}
	else return 0;
}

void printColor(uint8_t index) {
	char chBuffer[32]; //Just making sure the buffer will have enough space. 
	//Including the null character, this should only be 25 chars, but it could theoretically go up to 27.
	usart_prints("");
	sprintf(chBuffer,"Color %i: %i, %i, %i\r\n",index + 1, redVals[index], greenVals[index], blueVals[index]);
	usart_prints(chBuffer);
}
