/*
 * matchbox.cpp
 *
 *  Created on: Jul 10, 2016
 *      Author: jmiller
 */
#include <string.h>
#include <stm32f4xx.h> // chip-specific defines
#include <stm32f4xx_hal_dma.h>
#include <stm32f4xx_hal_tim.h>
#include <stm32f4xx_hal_pwr_ex.h>
#include "cmsis_os.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbserial.h"
#include "matchbox.h"
#include "gpio.h"
#include "pin.h"

extern "C" {
    TIM_HandleTypeDef htim1;
    RTC_HandleTypeDef hrtc;
}

typedef void (*fPtr)(void);
typedef struct _ClockConfig {
    public:
        uint32_t pllm;
        uint32_t plln;
        uint32_t pllp;
        uint32_t pllq;
        uint32_t ahbdiv;
        uint32_t apb1div;
        uint32_t apb2div;
        uint32_t latency;
} ClockConfig;

static const ClockConfig config[] = {
   // M   N   P              Q  AHBDIV           APB1DIV        APB2DIV        FLASH LATENCY
    { 8,  96, RCC_PLLP_DIV6, 4, RCC_SYSCLK_DIV2, RCC_HCLK_DIV1, RCC_HCLK_DIV1, FLASH_LATENCY_0 },
    { 8,  72, RCC_PLLP_DIV4, 3, RCC_SYSCLK_DIV2, RCC_HCLK_DIV1, RCC_HCLK_DIV1, FLASH_LATENCY_0 },
    { 8,  72, RCC_PLLP_DIV6, 3, RCC_SYSCLK_DIV1, RCC_HCLK_DIV1, RCC_HCLK_DIV1, FLASH_LATENCY_0 },
    { 8,  72, RCC_PLLP_DIV4, 3, RCC_SYSCLK_DIV1, RCC_HCLK_DIV1, RCC_HCLK_DIV1, FLASH_LATENCY_0 },
    { 8,  96, RCC_PLLP_DIV2, 4, RCC_SYSCLK_DIV2, RCC_HCLK_DIV2, RCC_HCLK_DIV1, FLASH_LATENCY_1 },
    { 8,  72, RCC_PLLP_DIV2, 3, RCC_SYSCLK_DIV1, RCC_HCLK_DIV2, RCC_HCLK_DIV1, FLASH_LATENCY_2 },
    { 8, 120, RCC_PLLP_DIV2, 5, RCC_SYSCLK_DIV1, RCC_HCLK_DIV4, RCC_HCLK_DIV2, FLASH_LATENCY_3 },
    { 8, 168, RCC_PLLP_DIV2, 7, RCC_SYSCLK_DIV1, RCC_HCLK_DIV4, RCC_HCLK_DIV1, FLASH_LATENCY_5 },
    { 16, 360, RCC_PLLP_DIV2, 8, RCC_SYSCLK_DIV1, RCC_HCLK_DIV4, RCC_HCLK_DIV1, FLASH_LATENCY_5 },
    { 12, 288, RCC_PLLP_DIV2, 8, RCC_SYSCLK_DIV1, RCC_HCLK_DIV4, RCC_HCLK_DIV4, FLASH_LATENCY_5 }
};

#ifdef USE_FULL_ASSERT
extern "C" void assert_failed(uint8_t* file, uint32_t line)
{
    printf("ASSERT: %s: line %d\n", file, line);
}
#endif

static void maybeJumpToBootloader() {
    const fPtr bootLoader = (fPtr) *(uint32_t*) 0x1fff0004;

    // Jump to bootloader if SW1 is low at reset
    pinInitInput(SW1_PIN);
    if (!readPin(SW1_PIN)) {
        pinInitOutput(LED_PIN, 1);
        for (int i = 0; i < 40; i++) {
            writePin(LED_PIN, i % 2);
            HAL_Delay(50);
        }
        SysTick->CTRL = 0; // Reset Systick timer
        SysTick->LOAD = 0;
        SysTick->VAL = 0;
        HAL_DeInit();
        HAL_RCC_DeInit();
        //__set_PRIMASK(1); // Disable interrupts - causes STM32f415 to fail entering DFU mode
        __HAL_RCC_GPIOC_CLK_DISABLE();
        __HAL_RCC_GPIOA_CLK_DISABLE();
        __HAL_RCC_GPIOB_CLK_DISABLE();
        __HAL_RCC_GPIOD_CLK_DISABLE();
        __set_MSP(0x20001000); // reset stack pointer to bootloader default
        bootLoader(); while (1);
    }
}

MatchBox::MatchBox(ClockSpeed clkSpeed) : _clkSpeed(clkSpeed) {
    HAL_Init();
    gpioInit();
    maybeJumpToBootloader();
    systemClockConfig(_clkSpeed);
    rtcInit();
    UsbSerial::getInstance(); // force initialization here
}

MatchBox::~MatchBox() {
}

void MatchBox::blinkOfDeath(Pin& led, BlinkCode code)
{
    while (1) {
        for (int i = 0; i < code; i++) {
            led.write(1);
            osDelay(125);
            led.write(0);
            osDelay(125);
        }
        osDelay(1000);
    }
}

uint32_t MatchBox::getTimer() {
    return HAL_GetTick();
}

void MatchBox::gpioInit(void) {
    // GPIO Ports Clock Enable
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    // POWER_PIN is wired to the LTC2954 KILL# pin. It must be remain high or power will shut off.
//    pinInitOutput(POWER_PIN, 1);
}

void MatchBox::systemClockConfig(ClockSpeed speed) {

    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
    _clkSpeed = speed;

    __HAL_RCC_PWR_CLK_ENABLE();

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = config[_clkSpeed].pllm;
    RCC_OscInitStruct.PLL.PLLN = config[_clkSpeed].plln;
    RCC_OscInitStruct.PLL.PLLP = config[_clkSpeed].pllp;
    RCC_OscInitStruct.PLL.PLLQ = config[_clkSpeed].pllq;
    HAL_RCC_OscConfig(&RCC_OscInitStruct);

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1
            | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = config[_clkSpeed].ahbdiv;
    RCC_ClkInitStruct.APB1CLKDivider = config[_clkSpeed].apb1div;
    RCC_ClkInitStruct.APB2CLKDivider = config[_clkSpeed].apb2div;
    HAL_RCC_ClockConfig(&RCC_ClkInitStruct, config[_clkSpeed].latency);
    const uint32_t freq = HAL_RCC_GetHCLKFreq();
    HAL_SYSTICK_Config(freq / 1000);

    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

    if (freq <= 144000000) {
        // Save a little power if clock is below 144MHz
        __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
    } else {
        // This should be the default
        __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    }

    /* SysTick_IRQn interrupt configuration TODO: why is this here? */
    HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}

void MatchBox::rtcInit(void) {
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };

#if 1 // Use LSE (Time base = ((31 + 1) * (0 + 1)) / 32.768Khz = ~1ms)
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
#else // Use LSI (Time base = ((4096 + 1) * (7 + 1)) / 32MHz = ~1ms)
    #define RTC_ASYNCH_PREDIV       7U
    #define RTC_SYNCH_PREDIV        4095U
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSI;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    RCC_OscInitStruct.LSIState = RCC_LSI_ON;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSI;
#endif

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) == HAL_OK) {
        PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
        if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) == HAL_OK) {
            /* Enable RTC Clock */
            __HAL_RCC_RTC_ENABLE();
            /* The time base should be 1ms
             Time base = ((RTC_ASYNCH_PREDIV + 1) * (RTC_SYNCH_PREDIV + 1)) / RTC_CLOCK
             HSE as RTC clock
             Time base = ((99 + 1) * (9 + 1)) / 1Mhz
             = 1ms
             LSE as RTC clock
             Time base = ((31 + 1) * (0 + 1)) / 32.768Khz
             = ~1ms
             LSI as RTC clock
             Time base = ((31 + 1) * (0 + 1)) / 32Khz
             = 1ms
             */
            hrtc.Instance = RTC;
            hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
            hrtc.Init.AsynchPrediv = RTC_ASYNCH_PREDIV;
            hrtc.Init.SynchPrediv = RTC_SYNCH_PREDIV;
            hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
            hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
            hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

            HAL_RTC_Init(&hrtc);

            /* Disable the write protection for RTC registers */
            __HAL_RTC_WRITEPROTECTION_DISABLE(&hrtc);

            /* Disable the Wake-up Timer */
            __HAL_RTC_WAKEUPTIMER_DISABLE(&hrtc);

            /* In case of interrupt mode is used, the interrupt source must disabled */
            __HAL_RTC_WAKEUPTIMER_DISABLE_IT(&hrtc, RTC_IT_WUT);

            /* Wait till RTC WUTWF flag is set  */
            uint32_t counter = 0;

            while (__HAL_RTC_WAKEUPTIMER_GET_FLAG(&hrtc, RTC_FLAG_WUTWF) == RESET) {
                if (counter++ == (SystemCoreClock / 48U)) {
                    return; // HAL_ERROR;
                }
            }

            /* Clear PWR wake up Flag */
            __HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

            /* Clear RTC Wake Up timer Flag */
            __HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
        }
        __HAL_RCC_RTC_ENABLE(); /* Enable RTC Clock */
    }
//    HAL_PWR_EnableBkUpAccess();
//    __HAL_RTC_WRITEPROTECTION_DISABLE(&hrtc);
//    __HAL_RCC_PWR_CLK_ENABLE();
////    __HAL_RCC_RTC_CONFIG(RCCEx_Periph_Clock_Selection);
//    __HAL_RCC_RTC_ENABLE();
//    HAL_RTC_Init(&hrtc);
}

