/*
 * matchbox.h
 *
 *  Created on: Jul 10, 2016
 *      Author: jmiller
 */

#ifndef MATCHBOX_H_
#define MATCHBOX_H_

#include "usbd_core.h"

class MatchBox {
    public:
        MatchBox();
        ~MatchBox();
        SPI_HandleTypeDef* getSpi1() { return &hspi1; }
        SPI_HandleTypeDef* getSpi2() { return &hspi2; }
        ADC_HandleTypeDef* getAdc1() { return &hadc1; }
    private:
        void gpioInit(void);
        void systemClockConfig(void);
        void spi1Init(void);
        void spi2Init(void);
        void adc1Init(void);
        void i2c1Init(void);
        void usart1Init(void);
        void usart2Init(void);

        SPI_HandleTypeDef hspi1;
        SPI_HandleTypeDef hspi2;
        UART_HandleTypeDef huart1;
        UART_HandleTypeDef huart2;
        ADC_HandleTypeDef hadc1;
        I2C_HandleTypeDef hi2c1;
};

#endif /* MATCHBOX_H_ */
