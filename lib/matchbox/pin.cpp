/*
 * button.cpp
 *
 *  Created on: Jul 16, 2016
 *      Author: jmiller
 */
#include "stm32f4xx.h"
#include "stm32f4xx_hal.h"
#include "gpio.h"
#include "pin.h"

// TODO: These are probably device-dependent
static const IRQn_Type irqMap[] = {
    EXTI0_IRQn, EXTI1_IRQn, EXTI2_IRQn, EXTI3_IRQn, EXTI4_IRQn,
    EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn, EXTI9_5_IRQn,
    EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn, EXTI15_10_IRQn,
};

inline uint32_t irqNumber(uint32_t pin) {
    return pin & 0x0f;
}

Pin* Pin::_pins[16];

Pin::Pin(int pin, const Config& config, Callback cb, void* arg)
        :_pin(pin), _config(config), _callback(cb), _arg(arg) {
    switch (config.mode) {
        case MODE_INPUT: _gpiox.Mode = GPIO_MODE_INPUT; break;
        case MODE_OUTPUT: _gpiox.Mode = GPIO_MODE_OUTPUT_PP; break;
        case MODE_ALTERNATE: _gpiox.Mode = GPIO_MODE_AF_PP; break;
        case MODE_ANALOG: _gpiox.Mode = GPIO_MODE_ANALOG; break;
    }
    if (_callback) { // enable interrupts
        switch (config.edge) {
            case EDGE_NONE: _gpiox.Mode |= 0; break; // disabled
            case EDGE_RISING: _gpiox.Mode |= GPIO_MODE_IT_RISING; break;
            case EDGE_FALLING: _gpiox.Mode |= GPIO_MODE_IT_FALLING; break;
            case EDGE_RISING_FALLING: _gpiox.Mode |= GPIO_MODE_IT_RISING_FALLING; break;
        }
    }
    _gpiox.Pull = config.pull == PULL_NONE ? GPIO_NOPULL
            : (config.pull == GPIO_PULLUP ? PULL_UP : PULL_DOWN);
    _gpiox.Pin = toIoPin(pin);
    switch(config.speed) {
        case SPEED_LOW: _gpiox.Speed = GPIO_SPEED_FREQ_LOW; break;
        case SPEED_MEDIUM: _gpiox.Speed = GPIO_SPEED_FREQ_MEDIUM; break;
        case SPEED_HIGH: _gpiox.Speed = GPIO_SPEED_FREQ_HIGH; break;
        case SPEED_VERY_HIGH: _gpiox.Speed = GPIO_SPEED_FREQ_VERY_HIGH; break;
    }
    HAL_GPIO_Init(toBus(pin), &_gpiox);

    const int irqno = irqNumber(pin);
    const IRQn_Type irq = irqMap[irqno];
    if (_callback) {
        HAL_NVIC_SetPriority(irq, 15 /* low preempt priority */, 0 /* high sub-priority*/);
        HAL_NVIC_EnableIRQ(irq);
    }
    if (_pins[irqno]) {
        delete _pins[irqno];
    }
    _pins[irqno] = this;
}

Pin::~Pin() {
    HAL_NVIC_DisableIRQ(irqMap[irqNumber(_pin)]);
    HAL_GPIO_DeInit(toBus(_pin), toIoPin(_pin));
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
        if (((1<<i) & GPIO_Pin) && Pin::_pins[i]) {
            Pin::_pins[i]->handleIrq();
        }
    }
}
