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
#include "usb_device.h"
#include "matchbox.h"
#include "font.h"
#include "lcd.h"

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
#define SW1_PIN PC13 // redefined to PC1 later
#define SW2_PIN PA13

//#define VERSION_01

#ifdef VERSION_01
#define LED_PIN SW1_PIN // Oops. Wired to PC13
#define POWER_PIN PD2
#else
#define LED_PIN PB2
#define POWER_PIN PA14
#endif

const float VREF = 3.3f;
const float VREF_CALIBRATION = 4.65f / 4.63f; // measured value, probably due to R15/R16 tolerance

static int usbInitialized = 0;
static uint8_t* asFile = 0;
static uint32_t asLine = 0;
static uint32_t asCount = 0;
static uint16_t adcValue[128] = { 0 };
static uint8_t mode;
static uint64_t pupCheck; // result of pull-up check
static uint64_t shCheck; // result of short check
static uint64_t shArray; // pins affected
static Lcd lcd(hspi2);

void toggleLed() {
    static uint8_t data;
    // printf("ADC %x\n", adcValue);
    writePin(LED_PIN, (data++) & 1);
}

typedef void (*fPtr)(void);

void maybeJumpToBootloader() {
    const fPtr bootLoader = (fPtr) *(uint32_t*) 0x1fff0004;

    // Jump to bootloader if PC13 is low at reset
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

extern "C" void EXTI1_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(toIoPin(SW1_PIN));
    HAL_NVIC_ClearPendingIRQ(EXTI1_IRQn);
    printf("IRQ1!\n");
}

extern "C" void EXTI15_10_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(toIoPin(SW2_PIN));
    HAL_NVIC_ClearPendingIRQ(EXTI15_10_IRQn);
    mode++;
    float sum = 0;
    for (int i = 0; i < 128; i++) {
        sum += adcValue[i];
    }
    sum /= 128.0f;
    float vcc = VREF * VREF_CALIBRATION * sum / 4095.0f;
    vcc *= 2.0f; // measured voltage is half due to voltage divider
//    printf("%s(mode = %d) VCC=%f\n", __func__, mode, vcc);
//    printf("bits: %p %08x\n", roboto_bold_10_bits, *(uint32_t*)roboto_bold_10_bits);
}

// Hack to receive bytes. Looks like the STM implementation is woefully incomplete :/
extern "C" {
    int write(int, uint8_t*, int len);
}

extern "C" void doReceive(uint8_t* buff, uint32_t* len)
{
//    printf("rx:%p len=%d", buff, *len);
    write(1, buff, *len);
    mode = *buff - '0';
}

static int adcCount = 0;
extern "C" void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* adcHandle)
{
    adcValue[adcCount++] = HAL_ADC_GetValue(adcHandle);
    adcCount = adcCount > 127 ? 0 : adcCount;
}

extern "C" void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    if (hspi == &hspi1) {
        printf("%s: SPI1!!\n", __func__);
    } else if (hspi == &hspi2) {
//        printf("%s: LCD should refresh!\n", __func__);
        lcd.refresh();
    } else {
        printf("%s: Invalid SPI %p\n", __func__, hspi);
    }
}

extern "C" void SPI2_IRQHandler()
{
    HAL_NVIC_ClearPendingIRQ(SPI2_IRQn);
    HAL_SPI_IRQHandler(&hspi2);
}

extern "C" void ADC_IRQHandler(void)
{
    HAL_ADC_IRQHandler(&hadc1);
}

void checkIoPins(uint64_t* pullUpCheck, uint64_t* shortCheck, uint64_t* shortArray) {
    static const int pins[] = {
            PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
            PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,
            PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10, PC11, PC12, PC13, PC14, PC15,
    };

    for (int i = 0; i < sizeof(pins) / sizeof(pins[0]); i++) {
        pinInitInput(pins[i]); // init with pull-up
    }

    // Verify all pulled-up pins are logic '1'
    *pullUpCheck = 0ULL;
    for (int i = 0; i < Number(pins); i++) {
        *pullUpCheck |= readPin(pins[i]) ? (1ULL << pins[i]) : 0ULL;
    }

    // Pull one pin down at a time and look for other shorted pins
    *shortCheck = *shortArray = 0ULL;
    for (int k = 0; k < Number(pins); k++) {
        if (k == POWER_PIN) continue; // don't kill the power!
        pinInitOutput(pins[k], 0); // pull it down
        HAL_Delay(1); // wait for things to settle (10pF/10k = 100ns)
        const int drv = pins[k];
        for (int i = 0; i < Number(pins); i++) {
            const int rcv = pins[i];
            if (drv != rcv && (readPin(rcv) ? 1ULL : 0ULL) != ((*pullUpCheck >> rcv) & 1ULL)) {
                // oops, unexpected pull down
                *shortCheck |= 1ULL << drv; // driver
                *shortArray |= 1ULL << rcv; // receiver
                break;
            }
        }
        pinInitInput(pins[k]); // pull it back up
    }
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

  // Do low-level IO check
  checkIoPins(&pupCheck, &shCheck, &shArray);

  // POWER_PIN is wired to the LTC2954 KILL# pin. It must be remain high or power will shut off.
  pinInitOutput(POWER_PIN, 1);

  /* Configure the system clock */
  SystemClock_Config();

  MX_SPI1_Init();
  MX_SPI2_Init();
#ifndef VERSION_01
  MX_SDIO_SD_Init(); // Disable until rev 1.0 with pin reassignment
#endif
  MX_USART1_UART_Init();
  MX_I2C1_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  MX_USB_DEVICE_Init();
  usbInitialized = 1;

  /* init code for FATFS */
//  MX_FATFS_Init();


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

void drawBars(int vertical)
{
    for (int j = 0; j < 128; j++) {
        for (int i = 0; i < 128; i++) {
            uint8_t r, g, b;
            int index = vertical ? j : i;
            lcd.setPixel(i,j, (index >> 4) & 1, (index >> 5) & 1, (index >> 6) & 1);
        }
    }
}

void drawChecker(int scale)
{
    for (int j = 0; j < 128; j++) {
        for (int i = 0; i < 128; i++) {
            uint8_t pix = ((j>>(scale)) ^ (i>>(scale))) & 1;
            lcd.setPixel(i,j, pix, pix, pix);
        }
    }
}

void drawAdc(uint8_t r, uint8_t g, uint8_t b, bool useLine)
{
    lcd.clear(1,1,1);
    if (useLine) {
        for (int x0 = 0; x0 < 127; x0++) {
            int y0 = adcValue[x0] & 0x7f;
            int x1 = x0+1;
            int y1 = adcValue[x1] & 0x7f;
            lcd.line(x0, y0, x1, y1, r, g, b);
        }
    } else {
        for (int i = 0; i < 128; i++) {
            lcd.setPixel(i, adcValue[i] & 0x7f, r, g, b);
        }
    }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

void waitForUSB()
{
    HAL_Delay(1000); // TODO
}

/* StartDefaultTask function */

void StartDefaultTask(void const * argument)
{
  /* USER CODE BEGIN 5 */
  /* Infinite loop */
  pinInitOutput(LED_PIN, 1);
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

  pinInitIrq(SW1_PIN, 1);
  pinInitIrq(SW2_PIN, 1);

  waitForUSB();

  // Print any asserts that happened before USB was initialized
  if (asFile) {
      printf("ASSERT: %s:%d (count=%d)\n", asFile, asLine, asCount);
      asFile = 0;
      asCount = 0;
  }

  HAL_ADC_Start_IT(&hadc1);

  // Init LCD
  lcd.begin();
  lcd.clear(1,1,1);
  lcd.refresh(); // start refreshing

  printf("IOCheck: pup:%012llx sh:%012llx ary:%012llx\n", pupCheck, shCheck, shArray);

  int frame = 0;
  char buff[32];
  while (1) {
    toggleLed();
    sprintf(buff, "Frame%04d", frame);
    lcd.putString(buff, 0, 0);
    int tmp = mode % 36;
    if (tmp == 0) {
        lcd.clear(frame & 1, frame & 2, frame & 4);
    } else if (tmp < 3) {
        drawBars(tmp == 2);
    } else if (tmp < 12) {
        drawChecker(tmp == 11 ? (frame&0x7) : (tmp - 3));
    } else if (tmp < 28) {
        drawAdc(tmp&1, tmp&2, tmp&4, tmp >= 20);
    } else {
        lcd.circle(64,64,frame%64, (frame>>6) & 1, (frame>>7) & 1, (frame>>8) & 1);
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
extern "C" void assert_failed(uint8_t* file, uint32_t line)
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
