/*
 * TermProject
 *
 * Created: 4/23/2024 1:58:54 PM
 * Author : Jared Mulder, Steven Vanni
 */ 

#include <avr/io.h>
#include "i2cmaster.h"

//Preprocessor defines:
#define SENSOR_ADDR = 0x38; //May need to be shifted left 1 bit.
#define LED_ADDR = 0xE8;

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
uint8_t checkButton(void);

int main(void)
{
	//Initialization:
	for (uint8_t i = 0; i < 4; i++) writeColor(0, 0, 0); //Clear the color buffer so the LEDs don't display incorrect data.
	
	i2c_start(SENSOR_ADDR+I2C_WRITE);
	i2c_write(0x00);
	i2c_write(0x01); //Tell the color sensor to turn on.
	i2c_stop();

	//Main loop:
    while (1) {
		buttonState = (0x03 & (buttonState << 1)) | checkButton();
		if (buttonState == 0x01) {
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
		if ((PORTB >> 7) & 0x01) {
			acc++;
		} else {
			acc--;
		}
	}
	if (acc > 127) return 1;
	else return 0;
}