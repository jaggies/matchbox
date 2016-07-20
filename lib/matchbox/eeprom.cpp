/*
 * eeprom.cpp
 *
 *  Created on: Jul 20, 2016
 *      Author: jmiller
 */
#include <stm32f4xx.h>
#include <stm32f4xx_hal_conf.h>
#include "eeprom.h"

uint8_t Eeprom::read(uint16_t addr) const {
    HAL_PWR_EnableBkUpAccess();
    uint8_t result = ((uint8_t*) BKPSRAM_BASE)[addr];
    HAL_PWR_DisableBkUpAccess();
    return result;
}

void Eeprom::write(uint16_t addr, uint8_t data) const {
    if (addr > 4095) {
        return; // out of range
    }
    HAL_PWR_EnableBkUpAccess();
    ((uint8_t*) BKPSRAM_BASE)[addr] = data;
    HAL_PWR_DisableBkUpAccess();
}

