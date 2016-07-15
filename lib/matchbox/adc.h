/*
 * adc.h
 *
 *  Created on: Jul 15, 2016
 *      Author: jmiller
 */

#ifndef ADC_H_
#define ADC_H_

#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_adc.h"

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef*);
extern "C" void ADC_IRQHandler(void);

class Adc {
    public:
        enum Device { AD1=0, AD2, AD3 };
        typedef void (*SampleCallback)(const uint16_t* samples, int n, void *args);
        Adc(Device adc, int sampleCount = 1);
        ~Adc();

        void start(SampleCallback cb = 0, void* args = 0);
        void stop();
        // Returns latest sample in buffer
        uint16_t getValue() { return _sampleIndex > 0 ? _samples[_sampleIndex-1] : 0; }

    private:
        friend void ADC_IRQHandler(void);
        friend void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hspi);
        void conversionComplete(ADC_HandleTypeDef* adcHandle);
        void handleIrq();
        Device _device; // number of device
        ADC_HandleTypeDef _adc;
        SampleCallback _callback;
        void* _arg;
        uint16_t* _samples; // sample buffer
        uint16_t  _sampleIndex; // write index for next sample
        uint16_t  _sampleCount; // number of samples to buffer before calling callback
};

#endif /* ADC_H_ */
