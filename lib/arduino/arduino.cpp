#include "cmsis_os.h"
#include "arduino.h"
#include "gpio.h"
#include "pin.h"

static Button *button[16];

// Mapping from arduino-style pins to STM pins. TODO: map these to something reasonable
static uint8_t pinMap[] = {
    PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
    PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,
    PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10, PC11, PC12, PC13, _PC14, _PC15,
    PD0, PD1, PD2, PD3, PD4, PD5, PD6, PD7, PD8, PD9, PD10, PD11, PD12, PD13, PD14, PD15 };

uint8_t digitalPinToInterrupt(uint8_t pin) {
    return pinMap[pin] & 0x0f; // interrupt line is same as pin (e.g. PA11 means irq line 11)
}

static void doArduinoInterrupt(uint32_t button, void* data) {
    void (*fptr)(void) = (void (*)(void)) data;
    fptr();
}

void attachInterrupt(uint8_t number, void (*isr)(void), int state) {
    int irq = irqNumber(pinMap[number]);
    if (button[irq]) {
        delete button[irq];
        button[irq] = NULL;
    }
    // TODO: This lies about the pin number.. always assumes PA0..PA15
    button[irq] = new Button(irq, doArduinoInterrupt, (void*) isr);
}

void delay(unsigned long ms) {
    osDelay(ms);
}

volatile int __delay;
void delayMicroseconds(unsigned int us) {
    __delay = us*100;
    while (__delay--) // TODO
        ;
}

void detachInterrupt(uint8_t irq) {
    if (irq < 16 && button[irq]) {
        delete button[irq];
        button[irq] = NULL;
    }
}

void pinMode(uint8_t pin, uint8_t mode) {
    pin = pinMap[pin];
    GPIO_InitTypeDef  GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = toIoPin(pin);
    switch (mode) {
        case INPUT_PULLDOWN:
        case INPUT_PULLUP:
        case INPUT:
            GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
            GPIO_InitStruct.Pull = mode == INPUT ? GPIO_NOPULL
                    : (mode == INPUT_PULLUP ? GPIO_PULLUP : GPIO_PULLDOWN);
            break;
        case OUTPUT:
            GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
            break;
    }
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(toBus(pin), &GPIO_InitStruct);
}

void digitalWrite(uint8_t pin, uint8_t value) {
    pin = pinMap[pin];
    writePin(pin, value);
}

uint8_t digitalRead(uint8_t pin) {
    pin = pinMap[pin];
    return readPin(pin);
}
