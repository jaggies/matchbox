/*
 * arduino_print.h
 *
 *  Created on: Jul 20, 2016
 *      Author: jmiller
 */

#ifndef ARDUINO_PRINT_H_
#define ARDUINO_PRINT_H_
#include <stdio.h>

enum { DEC, HEX };

class ArduinoPrint {
    public:
        void print(const char* str) { printf(str); }
        void print(uint8_t v, int fmt = DEC) {
            printf(fmt == DEC ? "%d" : "%x", v); }
        void println(const char* str) { printf("%s\n", str); }
};

#endif /* LIB_COMPAT_ARDUINO_PRINT_H_ */
