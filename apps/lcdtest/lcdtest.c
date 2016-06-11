/**
  ******************************************************************************
  * File Name          : main.c
  * Description        : Main program body
  ******************************************************************************
  *
  * COPYRIGHT(c) 2016 STMicroelectronics
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include <assert.h>
#include "stm32f4xx.h" // chip-specific defines
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_sd.h"
#include "cmsis_os.h"
#include "fatfs.h"
#include "usb_device.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

SD_HandleTypeDef hsd;
HAL_SD_CardInfoTypedef SDCardInfo;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

osThreadId defaultTaskHandle;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_SPI2_Init(void);
static void MX_SDIO_SD_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
void StartDefaultTask(void const * argument);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/

/* USER CODE END PFP */

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */
#define SW1_BUS GPIOC
#define SW1_PIN GPIO_PIN_13
#define SW1A_BUS GPIOC // de-conflicted PC13 on newer hardware
#define SW1A_PIN GPIO_PIN_1
#define SW2_BUS GPIOA // PA13
#define SW2_PIN GPIO_PIN_13

//#define VERSION_01

#ifdef VERSION_01
#define LED_BUS SW1_BUS // Oops. Wired to PC13
#define LED_PIN SW1_PIN
#else
#define LED_BUS GPIOB // rewired to PB2
#define LED_PIN GPIO_PIN_2
#endif

// LCD
//#define SOFT_SPI
#define LCD_PEN_BUS GPIOB
#define LCD_PEN_PIN GPIO_PIN_9
#define LCD_SCLK_BUS GPIOB
#define LCD_SCLK_PIN GPIO_PIN_10
#define LCD_SI_BUS GPIOC
#define LCD_SI_PIN GPIO_PIN_3
#define LCD_SCS_BUS GPIOB
#define LCD_SCS_PIN GPIO_PIN_1
#define LCD_EXTC_BUS GPIOB
#define LCD_EXTC_PIN GPIO_PIN_4
#define LCD_DISP_BUS GPIOB
#define LCD_DISP_PIN GPIO_PIN_5
#define LCD_LINE_SIZE (CHAN*XRES/8) // size of a line in bytes
#define LCD_SIZE (YRES * LCD_LINE_SIZE) // size of lcd frame in bytes
#define XRES 128
#define YRES 128
#define CHAN 3 // 3 == color, 1 == mono

static int usbInitialized = 0;
static uint8_t* asFile = 0;
static uint32_t asLine = 0;
static uint32_t asCount = 0;
static uint16_t adcValue[128] = { 0 };
static uint8_t lcdBuff[LCD_SIZE];
static uint8_t mode;

void pinInitOutput(GPIO_TypeDef* bus, int pin, int initValue) {
    GPIO_InitTypeDef  GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(bus, &GPIO_InitStruct);
    HAL_GPIO_WritePin(bus, pin, initValue);
}

void pinInitInput(GPIO_TypeDef* bus, int pin)
{
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(bus, &GPIO_InitStruct);
}

void pinInitIrq(GPIO_TypeDef* bus, int pin, int falling) {
    GPIO_InitTypeDef GPIO_InitStruct = { 0 };
    GPIO_InitStruct.Mode = falling ? GPIO_MODE_IT_FALLING : GPIO_MODE_IT_RISING;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Pin = pin;
    GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
    HAL_GPIO_Init(bus, &GPIO_InitStruct);
}

void toggleLed() {
    static uint8_t data;
    // printf("ADC %x\n", adcValue);
    HAL_GPIO_WritePin(LED_BUS, LED_PIN, (data++) & 1);
}

#ifdef SOFT_SPI
static GPIO_TypeDef* bus[] =
    { LCD_PEN_BUS, LCD_SCLK_BUS, LCD_SI_BUS, LCD_SCS_BUS, LCD_EXTC_BUS, LCD_DISP_BUS };
static const int pin[] =
    { LCD_PEN_PIN, LCD_SCLK_PIN, LCD_SI_PIN, LCD_SCS_PIN, LCD_EXTC_PIN, LCD_DISP_PIN };
static const int lcd_defaults[] = { 1, 0, 0, 0, 0, 1 };
#else
static GPIO_TypeDef* bus[] =
    { LCD_PEN_BUS, LCD_SCS_BUS, LCD_EXTC_BUS, LCD_DISP_BUS };
static const int pin[] =
    { LCD_PEN_PIN, LCD_SCS_PIN, LCD_EXTC_PIN, LCD_DISP_PIN };
static const int lcd_defaults[] = { 1, 0, 0, 1 };
#endif

void lcdInit() {
    for (int i = 0; i < sizeof(bus) / sizeof(bus[0]); i++) {
        pinInitOutput(bus[i], pin[i], lcd_defaults[i]);
    }
}

volatile int dly;
void delay(int n)
{
    dly = n;
    while (dly--);
}

void sendByte(uint8_t b) {
#ifdef SOFT_SPI
    for (int i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(LCD_SCLK_BUS, LCD_SCLK_PIN, 0);
        HAL_GPIO_WritePin(LCD_SI_BUS, LCD_SI_PIN, (b >> i) & 1);
        HAL_GPIO_WritePin(LCD_SCLK_BUS, LCD_SCLK_PIN, 1);
    }
    HAL_GPIO_WritePin(LCD_SCLK_BUS, LCD_SCLK_PIN, 0);
#else
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi2, &b, 1, 1000);
    if (status != HAL_OK) {
        printf("sendByte(): error = %d\n", status);
    }
#endif
}

void sendBytes(uint8_t* data, uint16_t count) {
#ifdef SOFT_SPI
    for (uint16_t i = 0; i < count; i++) {
        sendByte(data[i]);
    }
#else
//    HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi2, data, count, 1000);
    HAL_StatusTypeDef status = HAL_SPI_Transmit_IT(&hspi2, data, count);
    if (status != HAL_OK) {
        printf("sendBytes(): error = %d\n", status);
    }
#endif
}

uint8_t swap(uint8_t x)
{
    uint8_t res = 0;
    for (int i = 0; i < 8; i++) {
        res = (res << 1) | (x & 1);
        x >>= 1;
    }
    return res;
}

// Single line format : SCS CMD ROW <data0..n> 0 0 SCS#
// Multi-line format : SCS CMD ROW <data0..n> IGNORED ROW <data0..127> ... SCS#
void lcdSendLine(uint8_t* buff, int row, int frame, int clear) {
    sendByte(swap(0x80 | (frame ? 0x40:0) | (clear ? 0x20 : 0)));
    sendByte(row+1); // first row is 1, not 0 and bitswapped :/
    sendBytes(buff, CHAN*XRES / 8);
}

// For manual update
void lcdUpdate(uint8_t * buffer) {
    static int clear = 1;
    static int frame = 0;
    HAL_GPIO_WritePin(LCD_SCS_BUS, LCD_SCS_PIN, 1); // cs
    for (int i = 0; i < 128; i++) {
        lcdSendLine(buffer + i*(CHAN*XRES/8), i, frame & 1, clear);
    }
    HAL_GPIO_WritePin(LCD_SCS_BUS, LCD_SCS_PIN, 0); // cs
    HAL_GPIO_WritePin(LCD_EXTC_BUS, LCD_EXTC_PIN, (frame++) & 0x01);
    clear = 0;
}

void lcdUpdateLineIrq() {
    static uint8_t row = 0;
    static uint8_t frame = 0;
    static uint8_t clear = 0; // TODO
    HAL_StatusTypeDef status;
    if (row == 0) {
        HAL_GPIO_WritePin(LCD_SCS_BUS, LCD_SCS_PIN, 1); // cs
    } else if (row == 128) {
        uint8_t zero = 0;
        status = HAL_SPI_Transmit(&hspi2, &zero, 1, 1000);
        assert(HAL_OK == status);
        row = 0;
        frame++;
        HAL_GPIO_WritePin(LCD_SCS_BUS, LCD_SCS_PIN, 0); // cs
        delay(100);
        HAL_GPIO_WritePin(LCD_SCS_BUS, LCD_SCS_PIN, 1); // cs
    }

    // Send one line
    uint8_t cmd = swap(0x80 | (frame ? 0x40:0) | (clear ? 0x20 : 0));
    status = HAL_SPI_Transmit(&hspi2, &cmd, 1, 1000);
    assert(HAL_OK == status);

    uint8_t tmpRow = row + 1; // first row starts at 1
    status = HAL_SPI_Transmit(&hspi2, &tmpRow, 1, 1000);
    assert(HAL_OK == status);

    status = HAL_SPI_Transmit_IT(&hspi2, lcdBuff + row * LCD_LINE_SIZE, LCD_LINE_SIZE);
    assert(HAL_OK == status);

    row++;
}

void lcdSetPixel(uint8_t* buffer, uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b)
{
    uint16_t rbitaddr = (y * XRES + x) * CHAN + 0;
    uint16_t rbyteaddr = rbitaddr / 8;
    uint16_t rbit = rbitaddr % 8;
    buffer[rbyteaddr] &= ~(1 << rbit);
    buffer[rbyteaddr] |= r ? (1 << rbit) : 0;

    uint16_t gbitaddr = (y * XRES + x) * CHAN + 1;
    uint16_t gbyteaddr = gbitaddr / 8;
    uint16_t gbit = gbitaddr % 8;
    buffer[gbyteaddr] &= ~(1 << gbit);
    buffer[gbyteaddr] |= g ? (1 << gbit) : 0;

    uint16_t bbitaddr = (y * XRES + x) * CHAN + 2;
    uint16_t bbyteaddr = bbitaddr / 8;
    uint16_t bbit = bbitaddr % 8;
    buffer[bbyteaddr] &= ~(1 << bbit);
    buffer[bbyteaddr] |= b ? (1 << bbit) : 0;
}

typedef void (*fPtr)(void);

void maybeJumpToBootloader() {
    const fPtr bootLoader = (fPtr) *(uint32_t*) 0x1fff0004;

    // Jump to bootloader if PC13 is low at reset
    pinInitInput(SW1_BUS, SW1_PIN | SW1A_PIN);
    if (!HAL_GPIO_ReadPin(SW1_BUS, SW1_PIN) || !HAL_GPIO_ReadPin(SW1_BUS, SW1A_PIN)) {
        pinInitOutput(LED_BUS, LED_PIN, 1);
        for (int i = 0; i < 40; i++) {
            HAL_GPIO_WritePin(LED_BUS, LED_PIN, i % 2);
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

void EXTI1_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(SW1_PIN);
    HAL_NVIC_ClearPendingIRQ(EXTI1_IRQn);
    printf("IRQ1!\n");
}

void EXTI15_10_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(SW2_PIN);
    HAL_NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    mode++;
    printf("%s(mode = %d)\n", __func__, mode);
}

// Hack to receive bytes. Looks like the STM implementation is woefully incomplete :/
void doReceive(uint8_t* buff, uint32_t* len)
{
//    printf("rx:%p len=%d", buff, *len);
    write(1, buff, *len);
    mode = *buff - '0';
}

static int adcCount = 0;
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* adcHandle)
{
    adcValue[adcCount++] = HAL_ADC_GetValue(adcHandle);
    adcCount = adcCount > 127 ? 0 : adcCount;
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &hspi1) {
        printf("%s: SPI1!!\n", __func__);
    } else if (hspi == &hspi2) {
        lcdUpdateLineIrq();
    } else {
        printf("%s: Invalid SPI %p\n", __func__, hspi);
    }
}

void SPI2_IRQHandler()
{
    HAL_NVIC_ClearPendingIRQ(SPI2_IRQn);
    HAL_SPI_IRQHandler(&hspi2);
}

void ADC_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&hadc1);
}

int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration----------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Init GPIO subsystem */
  MX_GPIO_Init();

  // PD2 is wired to the LTC2954 KILL# pin. It must be remain high or power will
  // shut off.
  pinInitOutput(GPIOD, GPIO_PIN_2, 1);

  /* Configure the system clock */
  SystemClock_Config();

  MX_SPI1_Init();
  MX_SPI2_Init();
  // MX_SDIO_SD_Init(); // Disable until rev 1.0 with pin reassignment
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();

  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 2048);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */


  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
  /* USER CODE END WHILE */

  /* USER CODE BEGIN 3 */

  }
  /* USER CODE END 3 */

  return 0;
}

/** System Clock Configuration
*/
void SystemClock_Config(void)
{

  RCC_OscInitTypeDef RCC_OscInitStruct;
  RCC_ClkInitTypeDef RCC_ClkInitStruct;

  __HAL_RCC_PWR_CLK_ENABLE();

  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 72;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 3;
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2);

  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);
}

/* ADC1 init function */
void MX_ADC1_Init(void)
{

  ADC_ChannelConfTypeDef sConfig;

    /**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
    */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T1_CC1;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfDiscConversion = 0;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = DISABLE;
  HAL_ADC_Init(&hadc1);

    /**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
    */
  sConfig.Channel = ADC_CHANNEL_15;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

}

/* I2C1 init function */
void MX_I2C1_Init(void)
{

  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  HAL_I2C_Init(&hi2c1);

}

/* SDIO init function */
void MX_SDIO_SD_Init(void)
{

  hsd.Instance = SDIO;
  hsd.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
  hsd.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
  hsd.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
  hsd.Init.BusWide = SDIO_BUS_WIDE_1B;
  hsd.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
  hsd.Init.ClockDiv = 0;

}

/* SPI1 init function */
void MX_SPI1_Init(void)
{

  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_HARD_OUTPUT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  HAL_SPI_Init(&hspi1);

}

/* SPI2 init function */
void MX_SPI2_Init(void)
{

  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16; // _16 = ~2.25MHz
  hspi2.Init.FirstBit = SPI_FIRSTBIT_LSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  HAL_SPI_Init(&hspi2);

}

/* USART1 init function */
void MX_USART1_UART_Init(void)
{

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

/* USART2 init function */
void MX_USART2_UART_Init(void)
{

  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart2);

}

/** Configure pins as
        * Analog
        * Input
        * Output
        * EVENT_OUT
        * EXTI
*/
void MX_GPIO_Init(void)
{

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* StartDefaultTask function */

void StartDefaultTask(void const * argument)
{
  /* init code for USB_DEVICE */
  MX_USB_DEVICE_Init();
  usbInitialized = 1;

  /* init code for FATFS */
  MX_FATFS_Init();

  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  pinInitOutput(LED_BUS, LED_PIN, 0);
  char buff[32];
  //printf("Stack ptr %08x\n", __get_MSP());

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 15 /* low preempt priority */, 0 /* high sub-priority*/);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 15 /* low preempt priority */, 0 /* high sub-priority*/);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(ADC_IRQn, 15 /* low preempt priority */, 0 /* high sub-priority*/);
  HAL_NVIC_EnableIRQ(ADC_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 15 /* low preempt priority */, 0 /* high sub-priority*/);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  HAL_NVIC_SetPriority(SPI2_IRQn, 15 /* low preempt priority */, 0 /* high sub-priority*/);
  HAL_NVIC_EnableIRQ(SPI2_IRQn);

  // Done in usbd_conf.c
  //HAL_NVIC_SetPriority(OTG_FS_IRQn, 15 /* low preempt priority */, 0 /* high sub-priority*/);
  //HAL_NVIC_EnableIRQ(OTG_FS_IRQn);

  pinInitIrq(SW1_BUS, SW1A_PIN, 1);
  pinInitIrq(SW2_BUS, SW2_PIN, 1);

  // Print any asserts that happened before USB was initialized
  if (asFile) {
      printf("ASSERT: %s:%d (count=%d)\n", asFile, asLine, asCount);
      asFile = 0;
      asCount = 0;
  }

  HAL_ADC_Start_IT(&hadc1);

  // Init LCD
  lcdInit();
  lcdUpdateLineIrq(); // Call this once to get IRQ updates going

  int frame = 0;
  while (1) {
//    toggleLed();
    int tmp = mode % 20;
    if (tmp > 11) {
        memset(lcdBuff, 0xff, LCD_SIZE);
        for (int i = 0; i < 128; i++) {
            lcdSetPixel(lcdBuff, i, adcValue[i] & 0x7f, tmp&1, tmp&2, tmp&4);
        }
    } else for (int i = 0; i < 128; i++) {
        for (int j = 0; j < 128; j++) {
            uint8_t r, g, b;
            switch (tmp) {
                case 0: r = frame & 1; g = frame & 2; b = frame & 4;
                break;
                case 1: r = (j >> 4) & 1; g = (j >> 5) & 1; b = (j >> 6) & 1;
                break;
                case 2: r = (i >> 4) & 1; g = (i >> 5) & 1; b = (i >> 6) & 1;
                break;
                case 3:
                case 4:
                case 5:
                case 6:
                case 7:
                case 8:
                case 9:
                case 10: r = g = b = ((j>>(tmp-3)) ^ (i>>(tmp-3))) & 1;
                break;
                case 11: r = g = b = ((j>>(frame&0x7)) ^ (i>>(frame&0x7))) & 1;
                break;
            }
            lcdSetPixel(lcdBuff, i, j, r, g, b);
        }
    }
    frame++;
  }
  /* USER CODE END 5 */
}

#ifdef USE_FULL_ASSERT

/**
   * @brief Reports the name of the source file and the source line number
   * where the assert_param error has occurred.
   * @param file: pointer to the source file name
   * @param line: assert_param error line source number
   * @retval None
   */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    if (usbInitialized) {
        printf("ASSERT: %s: line %d\n", file, line);
    } else {
        // print it later
        asFile = file;
        asLine = line;
        asCount++;
    }
  /* USER CODE END 6 */
}

#endif

/**
  * @}
  */

/**
  * @}
*/

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
