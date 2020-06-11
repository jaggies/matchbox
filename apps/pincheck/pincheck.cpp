/*
 * pincheck.cpp
 *
 *  Created on: Jul 15, 2016
 *      Author: jmiller
 */

#include <cstdio>
#include "matchbox.h"
#include "util.h"

uint64_t pups;
uint64_t drivers;
uint64_t receivers;
osThreadId defaultTaskHandle;

/**
 * Check pins for short:
 * pullUpCheck: the state of all pins during the initial pull-up. Should be all ones.
 * driver: non-zero driver bit means pin affected another pin when driven
 * receiver: list of all pins affected from drivers.
 */
void checkIoPins(uint64_t* pullUpCheck, uint64_t* driver, uint64_t* receiver) {
    static const int pins[] = {
        PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
        PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,
        PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10, PC11, PC12, PC13, PC14, PC15 };

    for (int i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        pinInitInput(pins[i]); // init with pull-up
    }

    // Verify all pulled-up pins are logic '1'
    *pullUpCheck = 0ULL;
    for (int i = 0; i < Number(pins); i++) {
        *pullUpCheck |= readPin(pins[i]) ? (1ULL << pins[i]) : 0ULL;
    }

    // Pull one pin down at a time and look for other shorted pins
    *driver = *receiver = 0ULL;
    volatile int dly;
    for (int k = 0; k < Number(pins); k++) {
        if (k == POWER_PIN)
            continue; // don't kill the power!
        pinInitOutput(pins[k], 0); // pull it down
//        HAL_Delay(1); // wait for things to settle (10pF/10k = 100ns)
        dly= 100000; while (dly--) ;
        const int drv = pins[k];
        for (int i = 0; i < Number(pins); i++) {
            const int rcv = pins[i];
            if (drv != rcv && (readPin(rcv) ? 1ULL : 0ULL) != ((*pullUpCheck >> rcv) & 1ULL)) {
                // oops, unexpected pull down
                *driver |= 1ULL << drv; // driver
                *receiver |= 1ULL << rcv; // receiver
                break;
            }
        }
        pinInitInput(pins[k]); // pull it back up
    }
}

void printPinNames(uint64_t pins, uint64_t expected) {
    for (uint64_t i = 0; i < 48; i++) {
        if ((pins & (1LL << i)) != (expected << i)) {
            printf("P%c%d", "ABCD"[i>>4], i&0xf);
            if (i != 47) printf(" ");
        }
    }
}

void StartDefaultTask(void const * argument) {
    printf("HCLK: %d\n", HAL_RCC_GetHCLKFreq());
    printf("pups: %012llx (", pups);
    printPinNames(pups, 1);
    printf(")\n");

    printf("drivers: %012llx (", drivers);
    printPinNames(drivers, 0);
    printf(")\n");

    printf("receivers: %012llx (", receivers);
    printPinNames(receivers, 0);
    printf(")\n");

    // LED
    GPIO_InitTypeDef  GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    int count = 1;
    while (1) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, (count++ & 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
        HAL_Delay(500);
    }
}

void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);
  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);
  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}

int main(void) {
    HAL_Init();
    SystemClock_Config();

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    checkIoPins(&pups, &drivers, &receivers);
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
