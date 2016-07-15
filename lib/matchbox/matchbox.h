/*
 * matchbox.h
 *
 *  Created on: Jul 10, 2016
 *      Author: jmiller
 */

#ifndef MATCHBOX_H_
#define MATCHBOX_H_

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "board.h"
#include "gpio.h"

class MatchBox {
    public:
        MatchBox();
        ~MatchBox();
    private:
        void gpioInit(void);
        void usartInit(void);
        void systemClockConfig(void);
        UART_HandleTypeDef huart1;
};

#endif /* MATCHBOX_H_ */
