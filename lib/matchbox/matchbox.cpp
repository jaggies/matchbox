/*
 * matchbox.cpp
 *
 *  Created on: Jul 10, 2016
 *      Author: jmiller
 */
#include <string.h>
#include "stm32f4xx.h" // chip-specific defines
#include "cmsis_os.h"
#include "usbd_desc.h"
#include "usbd_cdc.h"
#include "usbserial.h"
#include "matchbox.h"
#include "gpio.h"

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
    { 8,  96, RCC_PLLP_DIV6, 4, RCC_SYSCLK_DIV2, RCC_HCLK_DIV1, RCC_HCLK_DIV1, FLASH_LATENCY_0 },
    { 8,  72, RCC_PLLP_DIV4, 3, RCC_SYSCLK_DIV2, RCC_HCLK_DIV1, RCC_HCLK_DIV1, FLASH_LATENCY_0 },
    { 8,  72, RCC_PLLP_DIV4, 3, RCC_SYSCLK_DIV1, RCC_HCLK_DIV1, RCC_HCLK_DIV1, FLASH_LATENCY_1 },
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
    systemClockConfig();
    usartInit();
    UsbSerial::getInstance(); // force initialization here
}

MatchBox::~MatchBox() {
}

void MatchBox::gpioInit(void) {
    // GPIO Ports Clock Enable
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();

    // POWER_PIN is wired to the LTC2954 KILL# pin. It must be remain high or power will shut off.
    pinInitOutput(POWER_PIN, 1);
}

void MatchBox::systemClockConfig(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
    RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

    __HAL_RCC_PWR_CLK_ENABLE();

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

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

    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq() / 1000);

    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

    /* SysTick_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}

void MatchBox::usartInit() {
   bzero(&huart1, sizeof(huart1));
   huart1.Instance = USART1;
   huart1.Init.BaudRate = 115200;
   huart1.Init.WordLength = UART_WORDLENGTH_8B;
   huart1.Init.StopBits = UART_STOPBITS_1;
   huart1.Init.Parity = UART_PARITY_NONE;
   huart1.Init.Mode = UART_MODE_TX_RX;
   huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
   huart1.Init.OverSampling = UART_OVERSAMPLING_16;
   HAL_UART_Init(&huart1);
}

