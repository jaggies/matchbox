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

class Pin;

class MatchBox {
    public:
        enum BlinkCode { CODE_ASSERT = 1 , CODE_LAST};
        enum ClockSpeed { C16MHz=0, C18MHz, C24MHz, C36MHz, C48MHz, C72MHz,
            C120MHz, C168MHz, C180MHz, C192MHz };
        MatchBox(ClockSpeed = C168MHz);
        ~MatchBox();
        void systemClockConfig(ClockSpeed speed);

        // TODO: Clean this up.. add stdlib versions to get time/date
        void rtcInit(void);
        RTC_HandleTypeDef* getRtc() { return &_rtcHandle; }

        static void blinkOfDeath(Pin& led, BlinkCode code);
        static uint32_t getTimer();
    private:
        void gpioInit(void);
        void rtcConfig(void);
        ClockSpeed _clkSpeed;
        RTC_HandleTypeDef _rtcHandle;
};

#endif /* MATCHBOX_H_ */
