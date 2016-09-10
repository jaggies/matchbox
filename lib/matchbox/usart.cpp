/*
 * usart.cpp
 *
 *  Created on: Sep 7, 2016
 *      Author: jmiller
 */

#include <string.h>

#include "usart.h"

static USART_TypeDef* usartMap[] = { USART1, USART2, USART3, USART6 };
static uint32_t stopBits[] = { UART_STOPBITS_1, UART_STOPBITS_2 };
static uint32_t parity[] = {UART_PARITY_NONE, UART_PARITY_EVEN, UART_PARITY_ODD };

Usart::Usart(Instance instance, const Config& config) {
   bzero(&huart, sizeof(huart));
   huart.Instance = usartMap[instance];
   huart.Init.BaudRate = config.baud;
   huart.Init.WordLength = config.wordLength == 9 ? UART_WORDLENGTH_9B : UART_WORDLENGTH_8B;
   huart.Init.StopBits = stopBits[config.stopBits];
   huart.Init.Parity = parity[config.parity];
   huart.Init.Mode = UART_MODE_TX_RX;
   huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
   huart.Init.OverSampling = UART_OVERSAMPLING_16;
   HAL_UART_Init(&huart);
}

uint16_t
Usart::transmit(const uint8_t* data, uint16_t len) {
    return HAL_OK == HAL_UART_Transmit(&huart, const_cast<uint8_t*>(data), len, 1000) ? len : 0;
}

uint16_t
Usart::receive(uint8_t* data, uint16_t len) {
    return HAL_OK == HAL_UART_Receive(&huart, data, len, 1000) ? len : 0;
}
