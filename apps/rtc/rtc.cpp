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

#define TAG 0x32F2
// Use LSE (Time base = ((31 + 1) * (0 + 1)) / 32.768Khz = ~1ms)
#define RTC_ASYNCH_PREDIV       7U
#define RTC_SYNCH_PREDIV        4095

osThreadId defaultTaskHandle;

static RTC_HandleTypeDef RtcHandle;

void StartDefaultTask(void const * argument);

void buttonHandler(uint32_t pin, void* data) {
    int &count = *(int*) data;
    switch (pin) {
        case SW1_PIN:
            count++;
            printf("Hello, count = %d\n", count);
            break;
        default:
            printf("Pin not handled: %d\n", pin);
    }
}

static void RTC_CalendarConfig(void) {
    RTC_DateTypeDef sdatestructure;
    RTC_TimeTypeDef stimestructure;

    /* Set Date: Friday March 13th 2015 */
    sdatestructure.Year = 0x15;
    sdatestructure.Month = RTC_MONTH_MARCH;
    sdatestructure.Date = 0x13;
    sdatestructure.WeekDay = RTC_WEEKDAY_FRIDAY;

    if (HAL_RTC_SetDate(&RtcHandle, &sdatestructure, RTC_FORMAT_BCD) != HAL_OK) {
        printf("Error setting date\n");
    }

    /* Set Time: 02:00:00 */
    stimestructure.Hours = 0x02;
    stimestructure.Minutes = 0x00;
    stimestructure.Seconds = 0x00;
    stimestructure.TimeFormat = RTC_HOURFORMAT12_AM;
    stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetTime(&RtcHandle, &stimestructure, RTC_FORMAT_BCD) != HAL_OK) {
        printf("Error setting time\n");
    }

    HAL_RTCEx_BKUPWrite(&RtcHandle, RTC_BKP_DR0, TAG);
}

static void configRtc() {
    __HAL_RCC_RTC_ENABLE(); /* Enable RTC Clock */

    RCC_OscInitTypeDef RCC_OscInitLSE;

    RCC_OscInitLSE.OscillatorType = RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitLSE.LSEState = RCC_LSI_ON;

    if(HAL_RCC_OscConfig(&RCC_OscInitLSE) != HAL_OK){
         printf("Failed to init OSC\n");
    }

    RtcHandle.Instance = RTC;
    RtcHandle.Init.HourFormat = RTC_HOURFORMAT_24;
    RtcHandle.Init.AsynchPrediv = RTC_ASYNCH_PREDIV;
    RtcHandle.Init.SynchPrediv = RTC_SYNCH_PREDIV;
    RtcHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
    RtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    RtcHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

    __HAL_RTC_WRITEPROTECTION_DISABLE(&RtcHandle);
    __HAL_RTC_RESET_HANDLE_STATE(&RtcHandle);

    HAL_StatusTypeDef status;
    if ( (status = HAL_RTC_Init(&RtcHandle)) != HAL_OK) {
        printf("Error initializing RTC: %d\n", status);
    }

    if (HAL_RTCEx_BKUPRead(&RtcHandle, RTC_BKP_DR0) != TAG) {
        printf("Calendar memory wiped, resetting\n");
        RTC_CalendarConfig();
    }
}

int main(void) {
    MatchBox* mb = new MatchBox();

    osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 2048);
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */
    while (1)
        ;
    return 0;
}

void StartDefaultTask(void const * argument) {
    int count = 0;
    Pin led(LED_PIN, Pin::Config().setMode(Pin::MODE_OUTPUT));
    Button b1(SW1_PIN, buttonHandler, &count);
    Spi spi2(Spi::SP2, Spi::Config().setOrder(Spi::LSB_FIRST).setClockDiv(Spi::DIV32));
    Lcd lcd(spi2, Lcd::Config().setDoubleBuffered(true));

    lcd.begin();
    lcd.clear(1,1,1);
    lcd.putString("RTC initializing\n", 0, 0);

    osDelay(1000); // allow USB to start up so we get debugging logs.

    configRtc();

    while (1) {
        RTC_DateTypeDef sdatestructureget;
        RTC_TimeTypeDef stimestructureget;
        char buff[256];

        HAL_RTC_GetTime(&RtcHandle, &stimestructureget, RTC_FORMAT_BIN);
        HAL_RTC_GetDate(&RtcHandle, &sdatestructureget, RTC_FORMAT_BIN);

        sprintf(buff, "Time: %02d:%02d:%02d\nDate: %02d-%02d-%02d\n", stimestructureget.Hours,
                stimestructureget.Minutes, stimestructureget.Seconds, sdatestructureget.Month,
                sdatestructureget.Date, 2000 + sdatestructureget.Year);
        lcd.clear(1,1,1);
        lcd.putString(buff, 0, 0);
        lcd.swapBuffers();
        led.write(count++ & 1);
        osDelay(1000);
    }
}
