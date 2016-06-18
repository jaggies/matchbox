/*
 * matchbox.h
 *
 *  Created on: Jun 8, 2016
 *      Author: jmiller
 */

#ifndef MATCHBOX_H_
#define MATCHBOX_H_

#include <stdint.h>
#include "stm32f4xx.h" // chip-specific defines

enum {
    PA0 = 0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
    PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,
    PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10, PC11, PC12, PC13, PC14, PC15,
    PD0, PD1, PD2, PD3, PD4, PD5, PD6, PD7, PD8, PD9, PD10, PD11, PD12, PD13, PD14, PD15,
};

#ifdef __cplusplus
extern "C" {
#endif

void pinInitOutput(uint16_t pin, uint16_t initValue);
void pinInitInput(uint16_t pin);
void pinInitIrq(uint16_t pin, uint16_t falling);
void writePin(uint16_t pin, uint16_t value);
int readPin(uint16_t pin);
uint16_t toIoPin(uint16_t pin);

# ifdef __cplusplus
}
#endif

#define Number(a) (sizeof(a) / sizeof(a[0]))

#endif /* MATCHBOX_H_ */
