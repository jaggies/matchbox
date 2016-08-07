/*
 * adc.cpp
 *
 *  Created on: Jul 15, 2016
 *      Author: jmiller
 */

#include <string.h> // bzero
#include "stm32f4xx.h" // chip-specific defines
#include "stm32f4xx_hal.h"
#include "adc.h"
#include "util.h"

static ADC_TypeDef* const _deviceMap[] = { ADC1, ADC2, ADC3 };

// Used to quickly map an irq to an instance of Adc object
static Adc* _adcMap[3];

Adc::Adc(Device device, int sampleCount) :
        _device(device), _callback(0), _arg(0),
        _samples(new uint16_t[sampleCount]), _sampleCount(sampleCount), _sampleIndex(0) {
    // Configure  global features of the ADC (Clock, Resolution, Data Alignment and #conversion)
    bzero(&_adc, sizeof(_adc));
    _adc.Instance = _deviceMap[device];
    _adc.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV8;
    _adc.Init.Resolution = ADC_RESOLUTION_12B;
    _adc.Init.ScanConvMode = DISABLE;
    _adc.Init.ContinuousConvMode = ENABLE;
    _adc.Init.DiscontinuousConvMode = DISABLE;
    _adc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    _adc.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;
    _adc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    _adc.Init.NbrOfDiscConversion = 0;
    _adc.Init.NbrOfConversion = 1;
    _adc.Init.DMAContinuousRequests = ENABLE;
    _adc.Init.EOCSelection = DISABLE;
    HAL_ADC_Init(&_adc);

    // Configure ADC regular channel, rank in the sequencer and sample time.
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_15;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
    HAL_ADC_ConfigChannel(&_adc, &sConfig);
    _adcMap[device] = this;

    HAL_NVIC_SetPriority(ADC_IRQn, 15 /* low preempt priority */, 0 /* high sub-priority*/);
}

Adc::~Adc() {
    stop();
    _adcMap[_device] = NULL;
    delete [] _samples;
}

void Adc::start(SampleCallback cb, void* arg) {
    _callback = cb;
    _arg = arg;
    HAL_NVIC_EnableIRQ(ADC_IRQn);
    HAL_ADC_Start_IT(&_adc);
}

void Adc::stop() {
    HAL_NVIC_DisableIRQ(ADC_IRQn);
}

void Adc::conversionComplete(ADC_HandleTypeDef* adcHandle) {
    _samples[_sampleIndex++] = HAL_ADC_GetValue(&_adc);
    if (_sampleIndex >= _sampleCount) {
        if (_callback) {
            _callback(_samples, _sampleIndex, _arg);
        }
        _sampleIndex = 0;
    }
}

extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* adcHandle)
{
    for (int i = 0; i < Number(_adcMap); i++) {
        Adc* adc = _adcMap[i];
        if (adc && (&adc->_adc == adcHandle)) {
            adc->conversionComplete(adcHandle);
            break;
        }
    }
}

extern "C" void ADC_IRQHandler(void)
{
    for (int i = 0; i < Number(_adcMap); i++) {
        if (_adcMap[i]) {
            // TODO: figure out which ADC triggered and call this once?
            HAL_ADC_IRQHandler(&_adcMap[i]->_adc);
        }
    }
}
