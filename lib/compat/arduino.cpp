#include "arduino.h"

// To save space, store bank as ordinal instead of pointer
typedef struct _pin {
        uint8_t bank; // PA, PB, PC, PD, etc.
        uint8_t pin; // pin in bank, e.g. PA12
} pin_t;

static pin_t pinMap[] = {
        { 0, 0 }, { 0, 1 }, { 0, 2 }, { 0, 3 },
        { 0, 4 }, { 0, 5 }, { 0, 6 }, { 0, 7 },
        { 0, 8 }, { 0, 9 }, { 0, 10 }, { 0, 11 },
        { 0, 12 }, { 0, 13 }, { 0, 14 }, { 0, 15 },

        { 1, 0 }, { 1, 1 }, { 1, 2 }, { 1, 3 },
        { 1, 4 }, { 1, 5 }, { 1, 6 }, { 1, 7 },
        { 1, 8 }, { 1, 9 }, { 1, 10 }, { 1, 11 },
        { 1, 12 }, { 1, 13 }, { 1, 14 }, { 1, 15 },

        { 2, 0 }, { 2, 1 }, { 2, 2 }, { 2, 3 },
        { 2, 4 }, { 2, 5 }, { 2, 6 }, { 2, 7 },
        { 2, 8 }, { 2, 9 }, { 2, 10 }, { 2, 11 },
        { 2, 12 }, { 2, 13 }, { 2, 14 }, { 2, 15 }, };

void pinMode(uint8_t pin, uint8_t mode) {

}

void digitalWrite(uint8_t pin, uint8_t value) {

}

uint8_t digitalRead(uint8_t pin) {
    return 0;
}


