/*
 * hello.cpp
 *
 *  Created on: Jun 20, 2016
 *      Author: jmiller
 */

#include <cstdio>
#include "matchbox.h"
#include "pin.h"

osThreadId defaultTaskHandle;
void StartDefaultTask(void const * argument);

void buttonHandler(uint32_t pin, void* data) {
    int &count = *(int*) data;
    switch (pin) {
        case SW1:
            count++;
            printf("Hello, count = %d\n", count);
            break;
        default:
            printf("Pin not handled: %d\n", pin);
    }
}

int main(void) {
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

void StartDefaultTask(void const * argument) {
    Pin led(LED1, Pin::Config().setMode(Pin::MODE_OUTPUT));
    int count = 0;
    Button b1(SW1, buttonHandler, &count);

    while (1) {
        led.write(count++ & 1);
        osDelay(250);
    }
}
