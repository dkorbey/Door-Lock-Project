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
uint8_t timerCnt = 0;		// Delay Counter
uint8_t wrongTypeCnt = 0;	// Stores the wrong attempts of entering the pin
uint8_t buzzerStage = 0;	// Sets the buzzer stage 0: button press, 1: correct pin, 2: wrong pin, 3: door bell

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
	
	// Configure Timer/Counter2 to control and send PWM signals to buzzers
	// Enable interrupt and set the overflow prescaler to 16ms
	TIM2_overflow_16ms();
	TIM2_overflow_interrupt_enable();
	
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
//	Interrupt Handler for scanning keypad, getting the typed pin and then compare the pin
ISR(TIMER0_OVF_vect)
{
	volatile static char pressedKey = ' ';		// Pressed Key
	volatile static uint8_t pinDigitCnt = 0;	// Contains the index value of the pin
	volatile static uint8_t scanningStage = 0;	// Get Pin --> 0: None, 1: getPin, 2: Standby
	
	// Scan the Keypad
	pressedKey = keypad_scan();
	
	// Key Press Buzzer
	if(pressedKey != ' ')
		buzzerStage = 1;
	
	// If user pressed #
	if(pressedKey == '#' && scanningStage == 0)
	{
		ringDoorBell();
		// Wait 3s and than standby
		scanningStage = 2;
		timerStage = 2;
	}
	//If user pressed *
	else if(pressedKey == '*' && scanningStage == 0)
	{
		scanningStage = 1;			// Enable getPin
		timerStage = 1;		// Start 5 second timer
		pinDigitCnt = 0;	// Set pin input index to 0
						
		// Configure lcd
		lcd_clrscr();
		lcd_gotoxy(2,1);
		lcd_puts("--Enter the pin--");
	}
		
	// If getPin enabled get the typed pin
	if(scanningStage == 1)
	{
		// Scan the entered pin
		if(pressedKey != '*' && pressedKey != '#' && pressedKey!= ' ')
		{
			// Put the pressed key into inputPin var
			inPin[pinDigitCnt] = pressedKey;
				
			// Configure lcd
			lcd_gotoxy((pinDigitCnt + 8),2);
			lcd_putc(pressedKey);
				
			// Increase the counter
			pinDigitCnt++;
		}
		
		// If 5s is up or the user typed all the digits of the pin enter here
		// and compare typed pin with the correct ones
		if(timerStage == 0 || pinDigitCnt > 3)
		{	
			// Compare the typed pin and the correct pins
			inID = comparePins(inPin);
			
			// If user typed pin before the timer finish stop the timer			
			timerStage = 0;
			timerCnt = 0;
			
			// Typed pin is incorrect
			if(inID == -1)
			{
				wrongPin();
			}
			else if(inID >= 0 && inID < 4)
			{
				correctPin(inID);
			}
			
			pinDigitCnt = 0;
			scanningStage = 2;
			timerStage = 2;
		}
	}
	
	// Changing the status to the standby
	if(scanningStage == 2)
	{
		if(timerStage == 0)
		{
			scanningStage = 0;
			standby();
		}
	}
}

// Interrupt Handler for creating 5s and 3s timers
ISR(TIMER1_OVF_vect)
{
	char string1[2] = "  ";
	
	// Standby status for the counter
	if(timerStage == 0)
		timerCnt = 0;	
	// 5s Count
	else if(timerStage == 1)
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
		lcd_puts(itoa((6-timerCnt), string1, 10));
	}
	// 3s Count
	else if(timerStage == 2)
	{
		timerCnt++;
		if(timerCnt >= 4)
		{
			timerCnt = 0;
			timerStage = 0;
		}
		
		// Configure LCD
		lcd_gotoxy(2,0);
		lcd_puts("Remaining time: ");
		lcd_puts(itoa((4-timerCnt), string1, 10));
	}
}

// Interrupt Handler for creating PWM signals for buzzers
ISR(TIMER2_OVF_vect)
{
	volatile static uint8_t buzzerCnt = 0;
	
	// Buzzer at standby
	if(buzzerStage == 0)
	{
		GPIO_write_low(&PORTB, Buzzer);
	}
	
	// Button press buzzer
	else if(buzzerStage == 1)
	{
		GPIO_write_high(&PORTB, Buzzer);
		
		buzzerCnt++;
		if(buzzerCnt == 10)
		{
			buzzerCnt = 0;
			buzzerStage = 0;
		}
	}
	// Correct Pin Buzzer
	else if(buzzerStage == 2)
	{
		GPIO_write_high(&PORTB, Buzzer);
		
		buzzerCnt++;
		if(buzzerCnt == 50)
		{
			buzzerCnt = 0;
			buzzerStage = 0;
		}
	}
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
	GPIO_write_low(&PORTB, Relay);
	
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

void ringDoorBell() 
{
	// Clear the lcd screen
	lcd_clrscr();
	// Print to lcd screen
	lcd_gotoxy(0,2);
	lcd_puts("Door bell is ringed");
}

void correctPin(uint8_t ID)
{	
	// Unlock the door
	GPIO_write_high(&PORTB, Relay);	

	// Light up the green led
	GPIO_write_high(&PORTB, greenLed);
	
	// Correct Pin Buzzer
	buzzerStage = 2;
	
	// Clear the lcd screen
	lcd_clrscr();
	// Print to lcd screen
	lcd_gotoxy(0,1);
	lcd_puts("Correct pin.");
	lcd_gotoxy(0,2);
	lcd_puts("Hello ");
	lcd_puts(names[ID]);
	
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
				break;
			}
		}
		// If an active pin is found, stop comparing
		if(pinId != -1)
		break;
	}
	
	return pinId;
}