/*
 * matchbox.cpp
 *
 *  Created on: Jul 10, 2016
 *      Author: jmiller
 */
#include <string.h>
#include "stm32.h"
#include "cmsis_os.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbserial.h"
#include "matchbox.h"
#include "board.h"
#include "util.h"
#include "pin.h"

#define RTC_TAG 0xabdd // If this changes, it means we lost power, so reset calendar

extern "C" {
    TIM_HandleTypeDef htim1;
    uint32_t get_fattime(void);
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

#ifdef STM32F4XX
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
#else
#warning TODO: generate standard clocks for H7
static const ClockConfig *config;
#endif

#ifdef USE_FULL_ASSERT
extern "C" void assert_failed(uint8_t* file, uint32_t line)
{
    printf("ASSERT: %s: line %d\n", file, line);
}
#endif

MatchBox* MatchBox::_instance = nullptr;

static void maybeJumpToBootloader() {
    const fPtr bootLoader = (fPtr) *(uint32_t*) 0x1fff0004;

    // Jump to bootloader if SW1 is low at reset
    pinInitInput(SW1);
    if (!readPin(SW1)) {
        pinInitOutput(LED1, 1);
        for (int i = 0; i < 40; i++) {
            writePin(LED1, i % 2);
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

uint32_t get_fattime(void) {
    MatchBox* mb = MatchBox::getInstance();
    if (!mb->getRtc()->Instance) {
        mb->rtcInit();
    }
    RTC_TimeTypeDef time_s;
    RTC_DateTypeDef date_s;
    HAL_RTC_GetTime(mb->getRtc(), &time_s, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(mb->getRtc(), &date_s, RTC_FORMAT_BIN);
    return toFatTime(&time_s, &date_s);
}

MatchBox::MatchBox(ClockSpeed clkSpeed) : _clkSpeed(clkSpeed) {
    _instance = this;
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

	#ifdef STM32F4XX
    __HAL_RCC_PWR_CLK_ENABLE();
	#else
	#warning TODO: enable power clock
	#endif

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

void MatchBox::rtcConfig(void) {
    RTC_DateTypeDef sdatestructure = { 0 };
    RTC_TimeTypeDef stimestructure = { 0 };

    /* Set Date: Saturday June 23th 2020 */
    sdatestructure.Year = 0x20;
    sdatestructure.Month = RTC_MONTH_JUNE;
    sdatestructure.Date = 0x23;
    sdatestructure.WeekDay = RTC_WEEKDAY_SATURDAY;

    if (HAL_RTC_SetDate(&_rtcHandle, &sdatestructure, RTC_FORMAT_BCD) != HAL_OK) {
        error("Error setting date\n");
    }

    /* Set VCR Time: 12:00:00 */
    stimestructure.Hours = 0x12;
    stimestructure.Minutes = 0x00;
    stimestructure.Seconds = 0x00;
    stimestructure.TimeFormat = RTC_HOURFORMAT_24;
    stimestructure.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    stimestructure.StoreOperation = RTC_STOREOPERATION_RESET;

    if (HAL_RTC_SetTime(&_rtcHandle, &stimestructure, RTC_FORMAT_BCD) != HAL_OK) {
        error("Error setting time\n");
    }

    HAL_RTCEx_BKUPWrite(&_rtcHandle, RTC_BKP_DR0, RTC_TAG);
}

void MatchBox::rtcInit(void) {
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = { 0 };
    HAL_StatusTypeDef status;
    _rtcHandle = { 0 };

    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    PeriphClkInitStruct.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;

    _rtcHandle.Instance = RTC;
    _rtcHandle.Init.HourFormat = RTC_HOURFORMAT_24;
    _rtcHandle.Init.AsynchPrediv = RTC_ASYNCH_PREDIV;
    _rtcHandle.Init.SynchPrediv = RTC_SYNCH_PREDIV;
    _rtcHandle.Init.OutPut = RTC_OUTPUT_DISABLE;
    _rtcHandle.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
    _rtcHandle.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
    __HAL_RTC_RESET_HANDLE_STATE(&_rtcHandle);

    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) == HAL_OK) {
        __HAL_RCC_RTC_ENABLE(); /* Enable RTC Clock */
        // Check the reset flag before re-initializing clock. Otherwise,
        // we loose a fraction of a second for each press of the reset
        // button...  TODO: handle other reset conditions?
        if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST)) {
            if ( (status = HAL_RTC_Init(&_rtcHandle)) != HAL_OK) {
                error("Error initializing RTC: %d\n", status);
                return;
            }
            // Clear the reset flags otherwise they'll stay on.
            __HAL_RCC_CLEAR_RESET_FLAGS();
        }
    }

    if (HAL_RTCEx_BKUPRead(&_rtcHandle, RTC_BKP_DR0) != RTC_TAG) {
        debug("Calendar memory wiped, resetting\n");

        HAL_PWR_EnableBkUpAccess();
        rtcConfig();
        HAL_PWR_DisableBkUpAccess();
    }
}

