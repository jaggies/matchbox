/*
 * threadtest.cpp
 *
 *  Created on: June 9, 2020
 *      Author: jmiller
 */

#include "matchbox.h"

volatile int count = 1;

void blink(void const* argument) {
    GPIO_InitTypeDef  GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    while (1) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, (count & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        osThreadYield();
    }
    osThreadTerminate(osThreadGetId()); // should never get here. Clean up this task if we do
}

void doCount(void const* argument) {
    while (1) {
        osDelay(250);
        count++;
        osThreadYield();
    }
    osThreadTerminate(osThreadGetId()); // should never get here. Clean up this task if we do
}

int main(void) {
    MatchBox* mb = new MatchBox();

    // ???  osKernelInitialize();

    osThreadDef(blinkThread, blink, osPriorityNormal, 1, 2048);
    osThreadCreate(osThread(blinkThread), NULL);

    osThreadDef(countThread, doCount, osPriorityNormal, 1, 2048);
    osThreadCreate(osThread(countThread), NULL);

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */
    while (1)
        ;
}
