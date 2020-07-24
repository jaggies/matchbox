/*
 * arduino.h
 *
 *  Created on: Jun 8, 2016
 *      Author: jmiller
 */

#ifndef ARDUINO_H_
#define ARDUINO_H_

#include <stdint.h>
#include <stddef.h>
#include "stm32.h"
#include "eeprom.h"
#include "arduino_spi.h"
#include "arduino_serial.h"

extern "C" {

// Pin modes
enum { LOW = 0, HIGH = 1 };
enum { INPUT = 0, OUTPUT, INPUT_PULLUP, INPUT_PULLDOWN };

// Life cycle methods
extern void setup();
extern void loop();

// Compatible methods
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
uint8_t digitalRead(uint8_t pin);
uint8_t digitalPinToInterrupt(uint8_t pin);
void attachInterrupt(uint8_t irq, void (*isr)(void), int state);
void detachInterrupt(uint8_t irq);
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);

// SPI
#define LSBFIRST 0
#define MSBFIRST 1

#define SPI_CLOCK_DIV4 0x00
#define SPI_CLOCK_DIV16 0x01
#define SPI_CLOCK_DIV64 0x02
#define SPI_CLOCK_DIV128 0x03
#define SPI_CLOCK_DIV2 0x04
#define SPI_CLOCK_DIV8 0x05
#define SPI_CLOCK_DIV32 0x06

#define SPI_MODE0 0x00
#define SPI_MODE1 0x04
#define SPI_MODE2 0x08
#define SPI_MODE3 0x0C
}

extern Eeprom EEPROM;
extern SPIClass SPI;

#endif /* ARDUINO_H_ */
