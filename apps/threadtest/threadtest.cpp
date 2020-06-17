/*
 * threadtest.cpp
 *
 *  Created on: June 9, 2020
 *      Author: jmiller
 */

#include "matchbox.h"
#include "cmsis_os.h"

volatile int count = 1;

static osMessageQId osQueue;

#define MSG_BLINK 123

void blink(void const* argument) {
    GPIO_InitTypeDef  GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    while (1) {
        /* Wait until something arrives in the queue. */
        osEvent event = osMessageGet(osQueue, osWaitForever);

        if (event.status == osEventMessage) {
            if (event.value.v == MSG_BLINK) {
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
                osDelay(50);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
            }
        }
    }

    // should never get here. Clean up this task if we do.
    osThreadTerminate(osThreadGetId());
}

void doCount(void const* argument) {
    while (1) {
        osDelay(250);
        osMessagePut (osQueue, (uint32_t)MSG_BLINK, 0);
    }
    osThreadTerminate(osThreadGetId()); // should never get here. Clean up this task if we do
}

extern "C" void vApplicationIdleHook(void) {
    __WFE(); // sleep while waiting
}

int main(void) {
    MatchBox* mb = new MatchBox();

    // ???  osKernelInitialize();

    osThreadDef(blinkThread, blink, osPriorityNormal, 1, 2048);
    osThreadCreate(osThread(blinkThread), NULL);

    osThreadDef(countThread, doCount, osPriorityNormal, 1, 2048);
    osThreadCreate(osThread(countThread), NULL);

    osMessageQDef(osqueue, 1 /* queue length */, uint16_t);
    osQueue = osMessageCreate (osMessageQ(osqueue), NULL);

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */
    while (1)
        ;
}
