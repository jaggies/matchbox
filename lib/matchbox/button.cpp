/*
 * button.cpp
 *
 *  Created on: Jul 16, 2016
 *      Author: jmiller
 */
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "gpio.h"
#include "button.h"

// TODO: These are probably device-dependent
static const IRQn_Type irqMap[] = {
    EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn, EXTI4_IRQn,
    EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn,
    EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
};

inline uint32_t irqNumber(uint32_t pin) {
    return pin & 0x0f;
}

Button* Button::_buttons[16];

Button::Button(int pin, Callback cb, void* arg, Edge edge, Pull pull)
        :_pin(pin), _callback(cb), _arg(arg), _edge(edge), _pull(pull) {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Mode = edge == FALLING ? GPIO_MODE_IT_FALLING : GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = pull == PULL_NONE ? GPIO_NOPULL
            : (pull == GPIO_PULLUP ? PULL_UP : PULL_DOWN);
    GPIO_InitStruct.Pin = toIoPin(pin);
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    GPIO_TypeDef* bus = toBus(pin); // PA, PB, etc.
    HAL_GPIO_Init(bus, &GPIO_InitStruct);

    int irqno = irqNumber(pin);
    IRQn_Type irq = irqMap[irqno];
    HAL_NVIC_SetPriority(irq, 15 /* low preempt priority */, 0 /* high sub-priority*/);
    HAL_NVIC_EnableIRQ(irq);
    _buttons[irqno] = this; // TODO: detect collisions
}

Button::~Button() {
    HAL_NVIC_DisableIRQ(irqMap[irqNumber(_pin)]);
}

extern "C" void EXTI0_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(1 << 0);
}

extern "C" void EXTI1_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(1 << 1);
}

extern "C" void EXTI2_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(1 << 2);
}

extern "C" void EXTI3_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(1 << 3);
}

extern "C" void EXTI4_IRQHandler(void)
{
    HAL_GPIO_EXTI_IRQHandler(1 << 4);
}

extern "C" void EXTI9_5_IRQHandler(void)
{
    for (int i = 5; i < 9; i++) {
        HAL_GPIO_EXTI_IRQHandler(1 << i);
    }
}

extern "C" void EXTI15_10_IRQHandler(void)
{
    for (int i = 10; i < 16; i++) {
        HAL_GPIO_EXTI_IRQHandler(1 << i);
    }
}

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    for (int i = 0; i < 16; i++) {
        if ((1<<i) & GPIO_Pin) {
            Button::_buttons[i]->handleIrq();
        }
    }
}
