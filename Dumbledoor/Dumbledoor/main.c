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
#define redLed		PB6		// Red led indicates wrong Pin 
#define greenLed	PB7		// Green Led indicates correct Pin

/* Includes ----------------------------------------------------------*/
#include <avr/io.h>			// AVR device-specific IO definitions
#include <avr/interrupt.h>		// Interrupts standard C library for AVR-GCC
#include <stdlib.h>			// To use itoa() function
#include "timer.h"			// Timer library for AVR-GCC
#include "lcd.h"			// LCD library for AVR-GCC
#include "gpio.h"			// GPIO library for AVR-GCC
#include "keypad.h"			// Key pad library for AVR-GCC
#include "uart.h"			// UART library for AVR-GCC

/* Function declarations ---------------------------------------------*/
void standby();				// Put system to the standby state
void ringDoorBell();			// Rings the door bell
void correctPin(uint8_t ID);		// Put system to the correct pin state
void wrongPin();			// Put system to the wrong pin state
int8_t comparePins(char input[]);	// Compares the typed pin with the correct pins,
					// if correct returns the user ID if not returns -1
							
/* Global Variables --------------------------------------------------*/
char inPin[4] = "    ";			// Input Pin (the pin user pressed)
int8_t inID = -1;			// Input ID (the ID of the typed Pin, if pin is wrong the Id value is -1)
uint8_t timerStage = 0;			// Sets the stage of the delay. 0: No Counter, 1: 5s Counter, 2: 3s Counter
uint8_t timerCnt = 0;			// Delay Counter
uint8_t buzzerStage = 0;		// Sets the buzzer stage  0: Standby, 1: button press, 2: correct pin, 3: wrong pin, 4: door bell
uint8_t correctAttempts = 0;		// Number of total correct entries
uint8_t wrongAttempts = 0;		// Number of total wrong entries

// Correct pin values
const char pins[4][4] = {
	"3467",		// ID = 0
	"4324",		// ID = 1
	"1962",		// ID = 2
	"7034"		// ID = 3
};
// Names of the pin owners						
const char names[4][13] = {
	"Mr Harrman",	// ID = 0
	"Mrs Leyla",	// ID = 1
	"Mr Baglamac",	// ID = 2
	"Mr Demiroren"	// ID = 3
};

// Custom characters for the lcd display						
uint8_t customChar[16] = {
	// addr 0: Heart
	0b00000, 0b00000, 0b01010, 0b11111, 0b01110, 0b00100, 0b00000, 0b00000,
	// addr 1: Bell
	0b00000, 0b00100, 0b01110, 0b01110, 0b11111, 0b00100, 0b00000, 0b00000
};

int main(void)
{
	// Initialize the LCD Display
	lcd_init(LCD_DISP_ON);
	
	// Initialize the Key Pad
	keypad_init();
	
	/*Defining and Printing a custom characters*/
	// Set pointer to beginning of CGRAM memory
	lcd_command(1 << LCD_CGRAM);
	for (uint8_t i = 0; i < 16; i++)
	{
		// Store all new chars to memory line by line
		lcd_data(customChar[i]);
	}
	// Set DDRAM address
	lcd_command(1 << LCD_DDRAM);
	
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
	
	// Configure Timer/Counter1 for counting timers
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
// Interrupt Handler for scanning keypad, getting the typed pin and then compare the pin
ISR(TIMER0_OVF_vect)
{
	volatile static char pressedKey = ' ';		// Pressed Key
	volatile static uint8_t pinDigitCnt = 0;	// Contains the index value of the pin
	volatile static uint8_t scanningStage = 0;	// Scanning Stage --> 0: None, 1: getPin, 2: Standby
	
	// Scan the Keypad
	pressedKey = keypad_scan();
	
	// Key Press Buzzer
	if(pressedKey != ' ')
		buzzerStage = 1;
	
	// If user pressed #, ring the door bell
	if(pressedKey == '#' && scanningStage == 0)
	{
		ringDoorBell();
		// Wait 3s and then standby
		scanningStage = 2;
		timerStage = 2;
	}
	// If user pressed *, configure the system to get typed pin
	else if(pressedKey == '*' && scanningStage == 0)
	{
		scanningStage = 1;	// Enable getPin
		timerStage = 1;		// Start 5 second timer
		pinDigitCnt = 0;	// Set pin input index to 0
						
		// Configure lcd
		lcd_clrscr();
		lcd_gotoxy(2,1);
		lcd_puts("--Enter the pin--");
	}
		
	// If scanningStage is 1 get the typed pin
	if(scanningStage == 1)
	{
		// Scan the entered pin
		if(pressedKey != '*' && pressedKey != '#' && pressedKey!= ' ')
		{
			// Put the pressed key into inputPin var
			inPin[pinDigitCnt] = pressedKey;
				
			// Configure lcd
			lcd_gotoxy((pinDigitCnt + 8),2);
			lcd_putc('*');
				
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
			// Typed pin is correct
			else if(inID >= 0 && inID < 4)
			{
				correctPin(inID);
			}
		
			pinDigitCnt = 0;
			// Wait 3s then, configure system for standby stage
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
		GPIO_write_low(&PORTB, doorBell);
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
	// Wrong Pin Buzzer
	else if(buzzerStage == 3)
	{
		GPIO_write_high(&PORTB, Buzzer);
		
		buzzerCnt++;
		if((buzzerCnt % 10) == 0)
		{
			GPIO_toggle(&PORTB, Buzzer);
		}
		if(buzzerCnt == 50)
		{
			buzzerCnt = 0;
			buzzerStage = 0;
		}
	}
	// Door Bell Buzzer
	else if(buzzerStage == 4)
	{
		GPIO_write_high(&PORTB, doorBell);
		
		buzzerCnt++;
		if(buzzerCnt == 10)
			GPIO_toggle(&PORTB, doorBell);
		if(buzzerCnt == 15)
			GPIO_toggle(&PORTB, doorBell);
		if(buzzerCnt == 20)
			GPIO_toggle(&PORTB, doorBell);
		if(buzzerCnt == 30)
			GPIO_toggle(&PORTB, doorBell);
		if(buzzerCnt == 35)
			GPIO_toggle(&PORTB, doorBell);
		if(buzzerCnt == 40)
			GPIO_toggle(&PORTB, doorBell);
		if(buzzerCnt == 50)
			GPIO_toggle(&PORTB, doorBell);
		if(buzzerCnt == 60)
			GPIO_toggle(&PORTB, doorBell);
		if(buzzerCnt == 65)
			GPIO_toggle(&PORTB, doorBell);
		if(buzzerCnt == 70)
			GPIO_toggle(&PORTB, doorBell);
		if(buzzerCnt == 80)
			GPIO_toggle(&PORTB, doorBell);
		if(buzzerCnt == 85)
			GPIO_toggle(&PORTB, doorBell);
		if(buzzerCnt == 90)
			GPIO_toggle(&PORTB, doorBell);
		if(buzzerCnt == 100)
			GPIO_toggle(&PORTB, doorBell);
			
		if(buzzerCnt == 100)
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
	// Correct Pin Buzzer
	buzzerStage = 4;
	
	// Clear the lcd screen
	lcd_clrscr();
	// Print to lcd screen
	lcd_gotoxy(2,2);
	lcd_puts("Door bell is");
	lcd_gotoxy(2,3);
	lcd_puts("rang. ");
	lcd_putc(1);
	lcd_putc(1);
	
	// UART
	uart_puts("Door bell is rang.");
	uart_puts("\r\n");
}

void correctPin(uint8_t ID)
{	
	char string2[2] = "  ";
	
	// Unlock the door
	GPIO_write_high(&PORTB, Relay);	

	// Light up the green led
	GPIO_write_high(&PORTB, greenLed);
	
	// Correct Pin Buzzer
	buzzerStage = 2;
	
	// Update Correct Attempts
	correctAttempts++;
	
	// Clear the lcd screen
	lcd_clrscr();
	// Print to lcd screen
	lcd_gotoxy(2,1);
	lcd_puts("Correct pin.");
	lcd_gotoxy(2,2);
	lcd_puts("Hello ");
	lcd_putc(0);
	lcd_putc(0);
	lcd_gotoxy(2,3);
	lcd_puts(names[ID]);
	
	// UART
	uart_puts(names[ID]);
	uart_puts(" entered to the room!");
	uart_puts("\r\n");	
	uart_puts("Total Attempts: ");
	uart_puts("\r\n");
	uart_puts("Correct: ");
	uart_puts(itoa(correctAttempts, string2, 10));
	uart_puts("\r\n");
	uart_puts("Wrong: ");
	uart_puts(itoa(wrongAttempts, string2, 10));
	uart_puts("\r\n");
}

void wrongPin()
{	
	char string2[2] = "  ";
	
	// Light up the red led
	GPIO_write_high(&PORTB, redLed);
	
	// Correct Pin Buzzer
	buzzerStage = 3;
	
	// Update Wrong Attempts
	wrongAttempts++;
	
	// Clear the lcd screen
	lcd_clrscr();
	// Print to lcd screen
	lcd_gotoxy(2,2);
	lcd_puts("Wrong pin.");
	
	// UART
	uart_puts("Wrong attempt to enter!");
	uart_puts("\r\n");
	uart_puts("Total Attempts: ");
	uart_puts("\r\n");
	uart_puts("Correct: ");
	uart_puts(itoa(correctAttempts, string2, 10));
	uart_puts("\r\n");
	uart_puts("Wrong: ");
	uart_puts(itoa(wrongAttempts, string2, 10));
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
				pinId = a;
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
