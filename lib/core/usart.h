/*
 * usart.h
 *
 *  Created on: Sep 7, 2016
 *      Author: jmiller
 */

#ifndef USART_H_
#define USART_H_

#include "stm32.h"

class Usart {
    public:
        struct Config{
            enum Parity {NONE=0, EVEN, ODD };
            enum StopBits {SB1=0, SB2 };
            Config& setBaud(uint32_t b) { baud = b; return *this; }
            Config& setParity(Parity p) { parity = p; return *this; }
            Config& setWordLength(uint8_t w) { wordLength = w; return *this; }
            Config& setStopBits(StopBits s) { stopBits = s; return *this; }
            Config() : baud(9600), parity(NONE), wordLength(8), stopBits(SB1) { }
            uint32_t baud;
            Parity parity;
            uint8_t wordLength;
            StopBits stopBits;
        };
        enum Instance { US1=0, US2, US3, US6 };
        Usart(Instance instance, const Config& config);
        ~Usart() { }
        uint16_t transmit(const uint8_t* data, uint16_t len);
        uint16_t receive(uint8_t* data, uint16_t len);
    private:
        UART_HandleTypeDef huart;
};

#endif /* USART_H_ */
