/*
 * button.h
 *
 *  Created on: Jul 16, 2016
 *      Author: jmiller
 */

#ifndef PIN_H_
#define PIN_H_

extern "C" void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

class Pin {
    public:
        typedef void (*Callback)(uint32_t pin, void* data);

        enum Edge { EDGE_NONE=0, EDGE_RISING, EDGE_FALLING, EDGE_RISING_FALLING }; //NONE==disabled
        enum Mode { MODE_INPUT=0, MODE_OUTPUT, MODE_ANALOG, MODE_ALTERNATE};
        enum Pull { PULL_NONE=0, PULL_UP, PULL_DOWN };
        enum Speed { SPEED_LOW=0, SPEED_MEDIUM, SPEED_HIGH, SPEED_VERY_HIGH };

        class Config {
            public:
                friend class Pin;
                Config() : edge(EDGE_NONE), mode(MODE_INPUT), pull(PULL_NONE), speed(SPEED_LOW) { }
                ~Config() { }

                Config& setEdge(Edge e) { edge = e; return *this; }
                Config& setMode(Mode m) { mode = m; return *this; }
                Config& setPull(Pull p) { pull = p; return *this; }
                Config& setSpeed(Speed s) { speed = s; return *this; }
            private:
                Edge edge;
                Mode mode;
                Pull pull;
                Speed speed;
        };

        Pin(int pin, const Config& config, Callback cb = 0, void* args = 0);
        ~Pin();

        bool read() const { return readPin(_pin); }
        void write(bool set) const { writePin(_pin, set); }
    private:
        friend void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
        inline void handleIrq() { if (_callback) _callback(_pin, _arg); }
        uint32_t _pin;
        Callback _callback;
        void *_arg;
        static Pin* _pins[16];
};

// Button is a Pin with interrupts enabled on falling edge
class Button : public Pin {
    public:
        Button(int pin, Callback cb = 0, void* args = 0)
            : Pin(pin, Config().setMode(MODE_INPUT).setEdge(EDGE_FALLING), cb, args) { }

};
#endif /* PIN_H_ */
