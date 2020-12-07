/***********************************************************************
 * 
 * Keypad library for AVR-GCC.
 * ATmega328P (Arduino Uno), 16 MHz, AVR 8-bit Toolchain 3.6.2
 *
 * Copyright (c) 2020-2021 Demirkan K. Baglamac and Rasit Demiroren
 * This work is licensed under the terms of the MIT license.
 *
 **********************************************************************/

/* Includes ----------------------------------------------------------*/
#include "keypad.h"

/* Global Variables --------------------------------------------------*/
char keyPadChar[4][3] = {
	{'1','2','3'},
	{'4','5','6'},
	{'7','8','9'},
	{'*','0','#'}};

uint8_t rows[4] = {RN0,RN1,RN2,RN3};
uint8_t columns[3] = {CN0,CN1,CN2};

/* Function definitions ----------------------------------------------*/
void keypad_init() {
	//Set all columns to output
	GPIO_config_output(&DDRC, CN0);
	GPIO_config_output(&DDRC, CN1);
	GPIO_config_output(&DDRC, CN2);
	//Set all columns to high
	GPIO_write_high(&PORTC, CN0);
	GPIO_write_high(&PORTC, CN1);
	GPIO_write_high(&PORTC, CN2);
	
	//Set all rows to input with pull-up resistor
	GPIO_config_input_pullup(&DDRC, RN0);
	GPIO_config_input_pullup(&DDRC, RN1);
	GPIO_config_input_pullup(&DDRC, RN2);
	GPIO_config_input_pullup(&DDRC, RN3);
}

/*--------------------------------------------------------------------*/
uint8_t keypad_scan() {
	static uint8_t isKeyPressed = 0;
	uint8_t rowN = -1;          // Row Number
	uint8_t colN = -1;          // Column Number
	char pKey = ' ';           // Pressed Key
	
	for(uint8_t i = 0; i<3; i++)
	{
		
		//Set all columns to high
		GPIO_write_high(&PORTC, CN0);
		GPIO_write_high(&PORTC, CN1);
		GPIO_write_high(&PORTC, CN2);
	
		// Make current column low and set the variable
		GPIO_write_low(&PORTC, columns[i]);
		colN = i;
		
		// Check each row
		for(uint8_t j = 0; j<4; j++)
		{
			
			// If it is low the button is pressed (current row x low valued column)
			if(GPIO_read(&PINC, rows[j]) == 0)
			{
				rowN = j;
				// From the row and column number get the pressed key
				pKey = keyPadChar[rowN][colN];
			}
		}
		
		// If a key pressed stop the current scanning cycle
		if(pKey != ' ')
			break;
	}
	
	if(pKey != ' ' && isKeyPressed == 0)
	{
		isKeyPressed = 1;
		return pKey;
	}
	else if(pKey == ' ')
	{
		isKeyPressed = 0;
		return pKey;
	}	
	else
	{
		pKey = ' ';
		return pKey;
	}
	
}