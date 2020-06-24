/*
 * rtc.cpp
 *
 *  Created on: Jun 20, 2020
 *      Author: jmiller
 */

#include <cstdio>
#include "matchbox.h"
#include "lcd.h"
#include "pin.h"
#include "util.h"

osThreadId defaultTaskHandle;

void StartDefaultTask(void const * argument);

void buttonHandler(uint32_t pin, void* data) {
    int &count = *(int*) data;
    switch (pin) {
        case SW1_PIN:
            count++;
            debug("Hello, count = %d\n", count);
            break;
        default:
            debug("Pin not handled: %d\n", pin);
    }
}

int main(void) {
    MatchBox* mb = new MatchBox(MatchBox::C16MHz);

    // Enable RTC clock
    mb->rtcInit();

    osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 2048);
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), mb);

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */
    while (1)
        ;
    return 0;
}

void StartDefaultTask(void const * argument) {
    MatchBox* mb = (MatchBox*) argument;
    int count = 0;
    Pin led(LED_PIN, Pin::Config().setMode(Pin::MODE_OUTPUT));
    Button b1(SW1_PIN, buttonHandler, &count);
    Spi spi2(Spi::SP2, Spi::Config().setOrder(Spi::LSB_FIRST).setClockDiv(Spi::DIV32));
    Lcd lcd(spi2, Lcd::Config().setDoubleBuffered(true));

    lcd.begin();
    lcd.clear(1,1,1);
    lcd.putString("RTC initializing\n", 0, 0);

    // For verification.. this should happen before the above call returns
//    while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET) {
//        debug("Waiting for LSE to stabilize\n");
//        osDelay(100);
//    }

    // TODO: Check if GPIO pins need to be enabled (PC14 & PC15)
#ifdef DEBUG
    osDelay(1000); // FIXME: wait for USB otherwise this hangs
#endif

    debug("RCC->BDCR = %08x\n", RCC->BDCR);
    debug("LSE Source: %08x\n", __HAL_RCC_GET_RTC_SOURCE());

    while (1) {
        RTC_DateTypeDef sdatestructureget;
        RTC_TimeTypeDef stimestructureget;
        char buff[256];

        HAL_RTC_GetTime(mb->getRtc(), &stimestructureget, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(mb->getRtc(), &sdatestructureget, RTC_FORMAT_BIN);

        lcd.clear(1,1,1);
        sprintf(buff, "%02d:%02d:%02d\n", stimestructureget.Hours,
                stimestructureget.Minutes, stimestructureget.Seconds);
        lcd.setFont("roboto_bold_32");
        lcd.putString(buff, 0, 0);

        lcd.setFont("roboto_bold_14");
        sprintf(buff, "%02d-%02d-%02d", sdatestructureget.Month, sdatestructureget.Date,
                2000 + sdatestructureget.Year);
        lcd.putString(buff);

        lcd.swapBuffers();
        led.write(count++ & 1);
    }
}
