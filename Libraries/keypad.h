#ifndef KEYPAD_H_
#define KEYPAD_H_

/***********************************************************************
 * 
 * Keypad library for AVR-GCC.
 * ATmega328P (Arduino Uno), 16 MHz, AVR 8-bit Toolchain 3.6.2
 *
 * Copyright (c) 2019-2020 Demirkan K. Baglamac and Rasit Demiroren
 * This work is licensed under the terms of the MIT license.
 *
 **********************************************************************/

/**
 * @file  keypad.h
 * @brief Keypad module library for AVR-GCC.
 *
 * @details
 * The library contains functions for controlling a keypad with your AVR.
 *
 * @author
 * Demirkan Korbey Baglamac and Rasit Demiroren
 * 
 * @copyright (c) 2020-2021 Demirkan K. Baglamac and Rasit Demiroren
 * Programmed for the Digital Electronics 2 project.
 * This work is licensed under the terms of the MIT license.
 */

/* Includes ----------------------------------------------------------*/
#include <avr/io.h>         // AVR device-specific IO definitions
#include "gpio.h"           // GPIO library for AVR-GCC

/* Definitions -------------------------------------------------------*/
// Defining Row Pins
//RN --> Row Number
#define RN0 PC6
#define RN1 PC5
#define RN2 PC4
#define RN3 PC3

// Defining Column Pins
//CN --> Column Number
#define CN0 PC0
#define CN1 PC1
#define CN2 PC2

/* Function prototypes -----------------------------------------------*/
/**
 * @brief    Sets all row pins as 0utput and high, 
 *           sets all column pins as input with pull-up resistor
 * @return   none
 */
void keypad_init();

/**
 * @brief    Scans the keypad
 * @return   Returns the pressed key
 */
uint8_t  keypad_scan();

#endif /* KEYPAD_H_ */