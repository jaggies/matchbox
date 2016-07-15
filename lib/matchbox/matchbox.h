/*
 * matchbox.h
 *
 *  Created on: Jul 10, 2016
 *      Author: jmiller
 */

#ifndef MATCHBOX_H_
#define MATCHBOX_H_

#include "usbd_core.h"
#include "board.h"

class MatchBox {
    public:
        MatchBox();
        ~MatchBox();
    private:
        void gpioInit(void);
        void systemClockConfig(void);
        void i2c1Init(void);
        void usart1Init(void);
        void usart2Init(void);

        UART_HandleTypeDef huart1;
        UART_HandleTypeDef huart2;
        I2C_HandleTypeDef hi2c1;
};

#endif /* MATCHBOX_H_ */
