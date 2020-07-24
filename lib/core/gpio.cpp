/*
 * matchbox.cpp
 *
 *  Created on: Jun 8, 2016
 *      Author: jmiller
 */

#include <assert.h>
#include "stm32.h"
#include "cmsis_os.h"
#include "gpio.h"
#include "util.h"

static GPIO_TypeDef* _bus[] = {
        GPIOA, GPIOB, GPIOC, GPIOD, GPIOE
};

GPIO_TypeDef* toBus(uint16_t pin) {
    uint16_t bus = pin >> 4;
   // assert(b < (sizeof(_bus) / sizeof(_bus[0]))); // PA..PE
    return bus < Number(_bus) ? _bus[bus] : 0;
}

uint16_t toIoPin(uint16_t pin) {
    // TODO: Verify this works across devices. Ordinarily we'd use constants like GPIO_PIN_1
    return 1 << (pin & 0x0f);
}

void pinInitOutput(uint16_t pin, uint16_t initValue) {
    GPIO_InitTypeDef  GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = toIoPin(pin);
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_TypeDef* bus = toBus(pin); // PA, PB, etc.
    HAL_GPIO_Init(bus, &GPIO_InitStruct);
    HAL_GPIO_WritePin(bus, GPIO_InitStruct.Pin, initValue ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void pinInitInput(uint16_t pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = toIoPin(pin);
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_TypeDef* bus = toBus(pin); // PA, PB, etc.
    HAL_GPIO_Init(bus, &GPIO_InitStruct);
}

void pinInitIrq(uint16_t pin, uint16_t falling) {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Mode = falling ? GPIO_MODE_IT_FALLING : GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Pin = toIoPin(pin);
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_TypeDef* bus = toBus(pin); // PA, PB, etc.
    HAL_GPIO_Init(bus, &GPIO_InitStruct);
}

void writePin(uint16_t pin, bool set) {
    GPIO_TypeDef* bus = toBus(pin);
    if (bus) {
        HAL_GPIO_WritePin(toBus(pin), toIoPin(pin), set ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

int readPin(uint16_t pin) {
    GPIO_TypeDef* bus = toBus(pin);
    return bus ? HAL_GPIO_ReadPin(bus, toIoPin(pin)) : 0;
}
