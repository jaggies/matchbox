/*
 * threadtest.cpp
 *
 *  Created on: June 9, 2020
 *      Author: jmiller
 */

#include <cstdio>
#include "matchbox.h"
#include "cmsis_os.h"

volatile int count = 1;

static osMessageQId osQueue;

extern "C" {
// These are called by the kernel
void PreSleepHook(uint32_t* ulExpectedIdleTime);
void PostSleepHook(uint32_t *ulExpectedIdleTime);
}

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
                osDelay(5);
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

// Define in FreeRTOSConfig.h: configPRE_SLEEP_PROCESSING
void PreSleepHook(uint32_t* ulExpectedIdleTime)
{
    /* Disable sleep for some devices. */
    __HAL_RCC_GPIOB_CLK_SLEEP_DISABLE(); // pin detect

    __HAL_RCC_GPIOA_CLK_DISABLE();
}

// Define in FreeRTOSConfig.h: configPOST_SLEEP_PROCESSING
void PostSleepHook(uint32_t *ulExpectedIdleTime) {
}

static void ReconfigureGPIO(void)
{
  GPIO_InitTypeDef GPIO_InitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* Allow voltage scaling. Caution: this only works below critical frequency */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /* Configure all GPIO as analog to reduce power consumption */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOG_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOI_CLK_ENABLE();

  GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Pin = GPIO_PIN_All;

  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);
  HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);
  HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);
  HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);
  HAL_GPIO_Init(GPIOI, &GPIO_InitStruct);

  /* Disable GPIOs clock */
  __HAL_RCC_GPIOA_CLK_DISABLE();
//  __HAL_RCC_GPIOB_CLK_DISABLE(); // LED
  __HAL_RCC_GPIOC_CLK_DISABLE();
  __HAL_RCC_GPIOD_CLK_DISABLE();
  __HAL_RCC_GPIOE_CLK_DISABLE();
  __HAL_RCC_GPIOF_CLK_DISABLE();
  __HAL_RCC_GPIOG_CLK_DISABLE();
  __HAL_RCC_GPIOH_CLK_DISABLE();
  __HAL_RCC_GPIOI_CLK_DISABLE();
}

// Not sure if this is also needed.
extern "C" void vApplicationIdleHook(void) {
    __WFE(); // sleep while waiting
}

int main(void) {
    MatchBox* mb = new MatchBox(MatchBox::C16MHz);

    // Configure everything as input to save power
    ReconfigureGPIO();

    // except LED and Power
    pinInitOutput(POWER_PIN, 1);
    pinInitOutput(LED_PIN, 1);

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
