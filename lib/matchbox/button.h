/*
 * button.h
 *
 *  Created on: Jul 16, 2016
 *      Author: jmiller
 */

#ifndef BUTTON_H_
#define BUTTON_H_

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

class Button {
    public:
        enum Edge { RISING=0, FALLING };
        enum Pull { PULL_NONE=0, PULL_UP, PULL_DOWN };
        typedef void (*Callback)(uint32_t button, void* data);

        Button(int pin, Callback cb, void* args, Edge edge=FALLING, Pull pull=PULL_NONE);
        ~Button();
    private:
        friend void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
        inline void handleIrq() { if (_callback) _callback(_pin, _arg); }
        uint32_t _pin;
        Callback _callback;
        void *_arg;
        Edge _edge;
        Pull _pull;
        static Button* _buttons[16];
};

#endif /* BUTTON_H_ */
