/*
 * arduino_spi.cpp
 *
 *  Created on: Jul 22, 2016
 *      Author: jmiller
 */

#include <cstdint>
#include "stm32.h"
#include "spi.h"
#include "util.h"
#include "arduino_spi.h"


static Spi* arduino_spi = NULL; // the underlying implementation

SPIClass::SPIClass() {

}

void SPIClass::begin() {
    if (!arduino_spi) {
        // MODE0
        arduino_spi = new Spi(Spi::SP3, Spi::Config()
            .setPolarity(Spi::LOW)
            .setPhase(Spi::Rising)
            .setOrder(Spi::LSB_FIRST)
            .setSlaveSelect(Spi::SOFTWARE));
    }
}

void SPIClass::setBitOrder(unsigned char) {

}

void SPIClass::setClockDivider(unsigned char) {

}

void SPIClass::setDataMode(unsigned char) {

}

uint8_t SPIClass::transfer(uint8_t data) {
    uint8_t result = 0;
    if (arduino_spi) {
        arduino_spi->txrx(&data, &result, 1);
//        printf("txrx: %x %x\n", data, result);
    }
    return result;
}



