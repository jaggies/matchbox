/*
 * arduino_serial.h
 *
 *  Created on: Jul 20, 2016
 *      Author: jmiller
 */

#ifndef ARDUINO_SERIAL_H_
#define ARDUINO_SERIAL_H_

#include "arduino_print.h"

enum { SERIAL_5N1, SERIAL_6N1, SERIAL_7N1, SERIAL_8N1,
    SERIAL_5N2, SERIAL_6N2, SERIAL_7N2, SERIAL_8N2,
    SERIAL_5E1, SERIAL_6E1, SERIAL_7E1, SERIAL_8E1,
    SERIAL_5E2, SERIAL_6E2, SERIAL_7E2, SERIAL_8E2,
    SERIAL_5O1, SERIAL_6O1, SERIAL_7O1, SERIAL_8O1,
    SERIAL_5O2, SERIAL_6O2, SERIAL_7O2, SERIAL_8O2
};

class ArduinoSerial : public ArduinoPrint {
    public:
        void begin(unsigned long baud) {
            begin(baud, SERIAL_8N1);
        }
        void begin(unsigned long, uint8_t) { }
        void end() { }
        int available(void) { return 0; }
        int peek(void) { return 0; }
        int read(void) { return 0; }
        int availableForWrite(void) { return 0; }
        void flush(void) { }
        size_t write(uint8_t) { return 0; }
        inline size_t write(unsigned long n) {
            return write((uint8_t) n);
        }
        inline size_t write(long n) {
            return write((uint8_t) n);
        }
        inline size_t write(unsigned int n) {
            return write((uint8_t) n);
        }
        inline size_t write(int n) {
            return write((uint8_t) n);
        }
        operator bool() {
            return true;
        }
};

extern ArduinoSerial Serial;

#endif /* ARDUINO_SERIAL_H_ */
