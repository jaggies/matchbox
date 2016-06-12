/*
 * arduino.h
 *
 *  Created on: Jun 8, 2016
 *      Author: jmiller
 */

#ifndef ARDUINO_H_
#define ARDUINO_H_

extern "C" {
enum {INPUT, INPUT_PULLUP, OUTPUT};
void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
uint8_t digitalRead(uint8_t pin);
}

#endif /* ARDUINO_H_ */
