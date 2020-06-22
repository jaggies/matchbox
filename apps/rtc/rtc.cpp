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

#define RTC_TAG 0xabad // If this changes, it means we lost power, so reset calendar

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

    HAL_RTCEx_BKUPWrite(&RtcHandle, RTC_BKP_DR0, RTC_TAG);
}

static void configRtc() {
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };
    RCC_OscInitTypeDef RCC_OscInitLSE = { 0 };
    HAL_StatusTypeDef status;

    RCC_OscInitLSE.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitLSE.LSEState = RCC_LSE_ON;

    if((status = HAL_RCC_OscConfig(&RCC_OscInitLSE)) != HAL_OK){
        printf("Failed to init OSC: status=%d\n", status);
    } else {
        printf("Successfully initialized LSE!\n");
    }

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;

    // For verification.. this should happen before the above call returns
    while (__HAL_RCC_GET_FLAG(RCC_FLAG_LSERDY) == RESET) {
        printf("Waiting for LSE to stabilize\n");
        osDelay(1000);
    }

    // ------
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) == HAL_OK) {
        __HAL_RCC_RTC_ENABLE(); /* Enable RTC Clock */

        RtcHandle.Instance = RTC;
        RtcHandle.Init.HourFormat = RTC_HOURFORMAT_24;
        RtcHandle.Init.AsynchPrediv = RTC_ASYNCH_PREDIV;
        RtcHandle.Init.SynchPrediv = RTC_SYNCH_PREDIV;
        RtcHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
        RtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
        RtcHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

        if ( (status = HAL_RTC_Init(&RtcHandle)) != HAL_OK) {
            printf("Error initializing RTC: %d\n", status);
            return;
        }

        /* Disable the write protection for RTC registers */
        __HAL_RTC_WRITEPROTECTION_DISABLE(&RtcHandle);

        /* What does this do? */
        // __HAL_RTC_RESET_HANDLE_STATE(&RtcHandle);

        /* Disable the Wake-up Timer */
        __HAL_RTC_WAKEUPTIMER_DISABLE(&RtcHandle);

        /* In case of interrupt mode is used, the interrupt source must disabled */
        __HAL_RTC_WAKEUPTIMER_DISABLE_IT(&RtcHandle, RTC_IT_WUT);

        /* TODO ? Wait till RTC WUTWF flag is set  */
//        uint32_t counter = 0;
//        while (__HAL_RTC_WAKEUPTIMER_GET_FLAG(&RtcHandle, RTC_FLAG_WUTWF) == RESET) {
//            if (counter++ == (SystemCoreClock / 48U)) {
//                return; // HAL_ERROR;
//            }
//        }

        /* Clear PWR wake up Flag */
        __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

        /* Clear RTC Wake Up timer Flag */
        __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&RtcHandle, RTC_FLAG_WUTF);
    }

    if (HAL_RTCEx_BKUPRead(&RtcHandle, RTC_BKP_DR0) != RTC_TAG) {
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

    lcd.setFont("robot_bold_14");
    lcd.begin();
    lcd.clear(1,1,1);
    lcd.putString("RTC initializing\n", 0, 0);

    // TODO: Check if GPIO pins need to be enabled (PC14 & PC15)
    osDelay(1000); // FIXME: wait for USB otherwise this hangs

    configRtc();

    printf("RCC->BDCR = %08x\n", RCC->BDCR);
    printf("LSE Source: %08x\n", __HAL_RCC_GET_RTC_SOURCE());

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
    }
}
