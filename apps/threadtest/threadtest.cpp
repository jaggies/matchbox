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
        osDelay(50); // this could be a lot more efficient. Look into using kernel messages
    }
    osThreadTerminate(osThreadGetId()); // should never get here. Clean up this task if we do
}

void doCount(void const* argument) {
    while (1) {
        osDelay(250);
        count++;
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

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */
    while (1)
        ;
}
