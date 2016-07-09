/*
 * handlers.h
 *
 *  Created on: Jul 9, 2016
 *      Author: jmiller
 */

#ifndef HANDLERS_H_
#define HANDLERS_H_

#ifdef __cplusplus
 extern "C" {
#endif

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void DebugMon_Handler(void);
void SysTick_Handler(void);
void TIM1_UP_TIM10_IRQHandler(void);
void OTG_FS_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* HANDLERS_H_ */
