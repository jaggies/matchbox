/*
 * handlers.cpp
 *
 *  Created on: Jul 9, 2016
 *      Author: jmiller
 */

#include "stm32.h"
#include "matchbox_it.h"
#include "cmsis_os.h"

extern PCD_HandleTypeDef hpcd_USB_OTG_FS;
extern TIM_HandleTypeDef htim1;
void NMI_Handler(void) {
}

void HardFault_Handler(void) {
    while (1) {
    }
}

void MemManage_Handler(void) {
    while (1) {
    }
}

void BusFault_Handler(void) {
    while (1) {
    }
}

void UsageFault_Handler(void) {
    while (1) {
    }
}

void DebugMon_Handler(void) {
    while (1) {
    }
}

void SysTick_Handler(void) {
    osSystickHandler(); // TODO: rewrite to use either HAL or RTOS, depending on which lib is linked
}

void TIM1_UP_TIM10_IRQHandler(void) {
    HAL_TIM_IRQHandler(&htim1);
}

void OTG_FS_IRQHandler(void) {
    HAL_PCD_IRQHandler(&hpcd_USB_OTG_FS);
}

// Allow applications to override these default implementations
__weak void vApplicationIdleHook (void) {
}

__weak void PreSleepHook(uint32_t* ulExpectedIdleTime) {
}

__weak void PostSleepHook(uint32_t *ulExpectedIdleTime) {
}
