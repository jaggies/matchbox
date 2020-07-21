/*
 * stm32.h
 *
 *  Created on: Jul 21, 2020
 *      Author: jmiller
 *
 *      Generic include file to select between multiple STM32 devices (e.g. F4, H7, etc.)
 */

#ifndef LIB_BRIDGE_STM32_H_
#define LIB_BRIDGE_STM32_H_

#if defined(STM32F4XX)
#include "stm32f4xx_hal_conf.h"
#include "stm32f4xx_hal.h"
#elif defined(STM32H7XX)
#include "stm32h7xx_hal_conf.h"
#include "stm32h7xx_hal.h"
#else
#error "Unsupported STM32 part"
#endif

#endif /* LIB_BRIDGE_STM32_H_ */
