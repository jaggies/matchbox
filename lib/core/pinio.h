/*
 * pinio.h
 *
 *  Created on: Jun 8, 2016
 *      Author: jmiller
 */

#ifndef GPIO_H_
#define GPIO_H_

#include <stdint.h>
#include "stm32.h"

#include "pinmap.h"

#ifdef __cplusplus
extern "C" {
#endif

void pinInitOutput(uint16_t pin, uint16_t initValue);
void pinInitInput(uint16_t pin);
void pinInitIrq(uint16_t pin, uint16_t falling);
void writePin(uint16_t pin, bool set);
int readPin(uint16_t pin);
uint16_t toIoPin(uint16_t pin);
GPIO_TypeDef* toBus(uint16_t pin);
inline uint32_t irqNumber(uint32_t pin) { return pin & 0x0f; }

# ifdef __cplusplus
}
#endif

#endif /* GPIO_H_ */
