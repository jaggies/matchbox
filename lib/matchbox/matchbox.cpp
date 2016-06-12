/*
 * matchbox.cpp
 *
 *  Created on: Jun 8, 2016
 *      Author: jmiller
 */

#include <assert.h>
#include "stm32f4xx.h" // chip-specific defines
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_sd.h"
#include "cmsis_os.h"
#include "fatfs.h"
#include "usb_device.h"
#include "matchbox.h"

static GPIO_TypeDef* _bus[] = {
        GPIOA, GPIOB, GPIOC, GPIOD, GPIOE
};

GPIO_TypeDef* toBus(int x) {
    int b = x >> 4;
   // assert(b < (sizeof(_bus) / sizeof(_bus[0]))); // PA..PE
    return b > Number(_bus) ? 0 : _bus[b];
}

int toPin(int pin) {
    return pin & 0x0f;
}

void pinInitOutput(uint16_t pin, uint16_t initValue) {
    GPIO_InitTypeDef  GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = toPin(pin);
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_TypeDef* bus = toBus(pin); // PA, PB, etc.
    HAL_GPIO_Init(bus, &GPIO_InitStruct);
    HAL_GPIO_WritePin(bus, pin, initValue ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void pinInitInput(uint16_t pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = toPin(pin);
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
    GPIO_InitStruct.Pin = toPin(pin);
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_TypeDef* bus = toBus(pin); // PA, PB, etc.
    HAL_GPIO_Init(bus, &GPIO_InitStruct);
}

void writePin(uint16_t pin, uint16_t value) {
    GPIO_TypeDef* bus = toBus(pin);
    if (bus) {
        HAL_GPIO_WritePin(bus, toPin(pin), value ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

int readPin(uint16_t pin) {
    GPIO_TypeDef* bus = toBus(pin);
    return bus ? HAL_GPIO_ReadPin(bus, toPin(pin)) : 0;
}
