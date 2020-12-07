/***********************************************************************
 * 
 * Door Lock Project
 * Accept 4 digit pin code, and if you don't know the pin you can ring 
 * the door bell as well. Programmed for,
 * ATmega328P (Arduino Uno), 16 MHz, AVR 8-bit Toolchain 3.6.2
 *
 * Copyright (c) 2020-2021 Demirkan K. Baglamac, Rasit Demiroren
 * This work is licensed under the terms of the MIT license.
 * 
 **********************************************************************/

/* Definitions -------------------------------------------------------*/
#ifndef F_CPU
#define F_CPU 1600000
#endif
#define Relay		PB3
#define doorBell	PB4
#define Buzzer		PB5
#define redLed		PB6		// Red led indicates wrong Pin or an error
#define greenLed	PB7		// Green Led indicates correct Pin

/* Includes ----------------------------------------------------------*/
#include <avr/io.h>			// AVR device-specific IO definitions
#include <avr/interrupt.h>	// Interrupts standard C library for AVR-GCC
#include <stdlib.h>			// To use itoa() function
#include "timer.h"			// Timer library for AVR-GCC
#include "lcd.h"			// LCD library for AVR-GCC
#include "gpio.h"			// GPIO library for AVR-GCC
#include "keypad.h"			// Key pad library for AVR-GCC
#include "uart.h"			// UART library for AVR-GCC

/* Function declarations ---------------------------------------------*/
void standby();						// Put system to the standby state
void errorF();						// Error Function
void ringDoorBell();				// Rings the door bell
void correctPin(uint8_t ID);		// Put system to the correct pin state
void wrongPin();					// Put system to the wrong pin state
int8_t comparePins(char input[]);	//Compares the typed pin with the correct pins,
									// if correct returns the user ID if not returns -1
							
/* Global Variables --------------------------------------------------*/
char inPin[4] = "----";		// Input Pin (the pin user pressed)
int8_t inID = -1;			// Input ID (the ID of the typed Pin, if pin is wrong the Id value is -1)
uint8_t timerStage = 0;		// Sets the stage of the delay. 0: No Counter, 1: 5s Counter, 2: 3s Counter

const char pins[4][4] = {
						"1234",		// ID = 0
						"4324",		// ID = 1
						"1962",		// ID = 2
						"7034"		// ID = 3
						};
const char names[4][13] = {
						"Mr Harrman",	// ID = 0
						"Mrs Leyla",	// ID = 1
						"Mr Baglamac",	// ID = 2
						"Mr Demiroren"	// ID = 3
						};

int main(void)
{
	// Initialize the LCD Display
	lcd_init(LCD_DISP_ON);
	
	// Initialize the Key Pad
	keypad_init();

	// Configure the Leds as output and set low
	GPIO_config_output(&DDRB, greenLed);
	GPIO_config_output(&DDRB, redLed);
	GPIO_write_low(&PORTB, greenLed);
	GPIO_write_low(&PORTB, redLed);	
	
	// Configure the buzzer as output and set low
	GPIO_config_output(&DDRB, Buzzer);
	GPIO_write_low(&PORTB, Buzzer);
	
	// Configure the doorbell as output and set low
	GPIO_config_output(&DDRB, doorBell);
	GPIO_write_low(&PORTB, doorBell);
	
	// Configure Relay as output and set low
	GPIO_config_output(&DDRB, Relay);
	GPIO_write_low(&PORTB, Relay);	
	
	// Set the program to standby state
	standby();
	
    // Configure Timer/Counter0 for scanning the key pad
    // Enable interrupt and set the overflow prescaler to 4ms
    TIM0_overflow_4ms();
    TIM0_overflow_interrupt_enable();
	
	// Configure Timer/Counter1 for creating delays
	// Enable interrupt and set the overflow prescaler to 1s
	TIM1_overflow_1s();
	TIM1_overflow_interrupt_enable();
	
    // Initialize UART to asynchronous, 8N1, 9600
    uart_init(UART_BAUD_SELECT(9600, F_CPU));
	
    // Enables interrupts by setting the global interrupt mask
    sei();
	
    /* Replace with your application code */
    while (1) 
    {
    }
	
	// Will never reach this
	return 0;
}

/* Interrupt handlers ------------------------------------------------*/
ISR(TIMER0_OVF_vect)
{
	volatile static char pressedKey = ' ';		// Pressed Key
	volatile static uint8_t pinDigitCnt = 0;	// Contains the index value of the pin
	volatile static uint8_t getPin = 0;			// Get Pin --> 0: No, 1:Yes
	
	// Scan the Keypad
	pressedKey = keypad_scan();
	
	// If user pressed *
	if(pressedKey == '#' && getPin == 0)
	{
		// DoorBell
		// Wait 3s ??
		// Standby
	}
	//If user pressed #
	else if(pressedKey == '*' && getPin == 0)
	{
		getPin = 1;			// Enable getPin
		timerStage = 1;		// Start 5 second timer
		pinDigitCnt = 0;	// Set pin input index to 0
						
		// Configure lcd
		lcd_clrscr();
		lcd_gotoxy(2,1);
		lcd_puts("--Enter the pin--");
	}
		
	// If getPin enabled get the typed pin
	if(getPin == 1 && pressedKey!= ' ')
	{
		if(pressedKey != '*' && pressedKey != '#')
		{
			// Put the pressed key into inputPin var
			inPin[pinDigitCnt] = pressedKey;
				
			// Configure lcd
			lcd_gotoxy((pinDigitCnt + 8),2);
			lcd_putc(pressedKey);
				
			// Increase the counter
			pinDigitCnt++;
				
			if(timerStage == 0 || pinDigitCnt > 3)
			{
				getPin = 0;
				pinDigitCnt = 0;
					
				inID = comparePins(inPin);
					
				// Typed pin is incorrect
				if(inID == -1)
				{
					// If user typed pin before the timer finish stop the timer
					timerStage = 0;
					wrongPin();
					// Wait 3s
					// Standby
				}
				else if(inID >= 0 && inID < 4)
				{
					// If user typed pin before the timer finish stop the timer
					timerStage = 0;
					correctPin(inID);
					// Wait 3s
					// Standby
				}
			}
		}
		else
		{
			// error
			// Wait 3s
			// standby
		}
	}
}

ISR(TIMER1_OVF_vect)
{
	volatile static uint8_t timerCnt = 0;		// Delay Counter
	char string1[2] = "  ";
	
	// 5s count
	if(timerStage == 1)
	{
		timerCnt++;
		if(timerCnt >= 6)
		{
			timerCnt = 0;
			timerStage = 0;
		}
		
		// Configure LCD
		lcd_gotoxy(2,0);
		lcd_puts("Remaining time: ");
		if(timerCnt != 0)
			lcd_puts(itoa((6-timerCnt), string1, 10));
		else
			lcd_puts(itoa(timerCnt, string1, 10));
	}
	
	uart_puts(itoa(timerStage, string1, 10));
}

/* Function definitions ----------------------------------------------*/
void standby()
{
	// Reset input ID
	inID = -1;
	
	// Reset typed pin
	inPin[0] = ' ';
	inPin[1] = ' ';
	inPin[2] = ' ';
	inPin[3] = ' ';
	
	// Reset Leds
	GPIO_write_low(&PORTB, greenLed);
	GPIO_write_low(&PORTB, redLed);
	
	// Lock the door
	GPIO_write_low(&DDRB, Relay);
	
	// Clear the lcd screen
	lcd_clrscr();
	// Print to lcd screen
	lcd_gotoxy(2,0);
	lcd_puts("Dumbledoor wishes");
	lcd_gotoxy(4,1);
	lcd_puts("Magical Days!");
	lcd_gotoxy(1,2);
	lcd_puts("* --> Enter the pin");
	lcd_gotoxy(1,3);
	lcd_puts("# --> Door Bell");
}

void correctPin(uint8_t ID)
{	
	// Unlock the door
	GPIO_write_high(&PORTB, Relay);	

	// Light up the green led
	GPIO_write_high(&PORTB, greenLed);
	
	// Clear the lcd screen
	lcd_clrscr();
	// Print to lcd screen
	lcd_gotoxy(2,2);
	lcd_puts("Correct pin.");
	
	// UART
	uart_puts(names[ID]);
	uart_puts(" entered to the room!");
	uart_puts("\r\n");	
}

void wrongPin()
{
	// Light up the red led
	GPIO_write_high(&PORTB, redLed);
	
	// Clear the lcd screen
	lcd_clrscr();
	// Print to lcd screen
	lcd_gotoxy(2,2);
	lcd_puts("Wrong pin.");
	
	// UART
	uart_puts("Wrong attempt to enter!");
	uart_puts("\r\n");
}

int8_t comparePins(char input[])
{
	int8_t pinId = -1;	// Active pin ID, If -1 no active pins

	// Checking each registered pin
	// pins[0], pins[1], pins[2], pins[3]
	for(uint8_t a = 0; a<4; a++)
	{
		// Compare 4-digit input pin with the registers pin
		// Ex. pi
		for(uint8_t b = 0; b<4; b++)
		{
			if(input[b] == pins[a][b])
			{
				pinId = b;
			}
			else
			{
				pinId = -1;
			}
		}
		// If an active pin is found, stop comparing
		if(pinId != -1)
		break;
	}
	
	return pinId;
}