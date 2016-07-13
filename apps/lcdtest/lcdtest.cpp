/*
 * lcdtest.c
 *
 *  Created on: Jun 20, 2016
 *      Author: jmiller
 */

#include <assert.h>
#include "stm32f4xx.h" // chip-specific defines
#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include "gpio.h"
#include "matchbox.h"
#include "font.h"
#include "lcd.h"
#include "util.h"

//#define VERSION_01 // Define for CPU hardware version 0.1
static MatchBox *mb;
static Lcd* lcd;

osThreadId defaultTaskHandle;

void StartDefaultTask(void const * argument);

#define SW1_PIN PC13 // redefined to PC1 later
#define SW2_PIN PA13

#ifdef VERSION_01
#define LED_PIN SW1_PIN // Oops. Wired to PC13
#define POWER_PIN PD2
#else
#define LED_PIN PB2
#define POWER_PIN PA14
#endif

const float VREF = 3.3f;
const float VREF_CALIBRATION = 4.65f / 4.63f; // measured value, probably due to R15/R16 tolerance

static uint16_t adcValue[128] = { 0 };
static uint8_t mode;
static uint64_t pupCheck; // result of pull-up check
static uint64_t shCheck; // result of short check
static uint64_t shArray; // pins affected

#ifdef USE_FULL_ASSERT
extern "C" void assert_failed(uint8_t* file, uint32_t line)
{
    printf("ASSERT: %s: line %d\n", file, line);
}
#endif

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

extern "C" void ADC_IRQHandler(void)
{
    HAL_ADC_IRQHandler(mb->getAdc1());
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
  mb = new MatchBox();
  Spi* spi2 = new Spi(Spi::SP2);
  lcd = new Lcd(*spi2);

//  // Do low-level IO check
//  checkIoPins(&pupCheck, &shCheck, &shArray);

  // POWER_PIN is wired to the LTC2954 KILL# pin. It must be remain high or power will shut off.
  pinInitOutput(POWER_PIN, 1);

  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 2048);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  while (1)
      ;

  return 0;
}

void drawBars(int vertical)
{
    for (int j = 0; j < 128; j++) {
        for (int i = 0; i < 128; i++) {
            uint8_t r, g, b;
            int index = vertical ? j : i;
            lcd->setPixel(i,j, (index >> 4) & 1, (index >> 5) & 1, (index >> 6) & 1);
        }
    }
}

void drawChecker(int scale)
{
    for (int j = 0; j < 128; j++) {
        for (int i = 0; i < 128; i++) {
            uint8_t pix = ((j>>(scale)) ^ (i>>(scale))) & 1;
            lcd->setPixel(i,j, pix, pix, pix);
        }
    }
}

void drawAdc(uint8_t r, uint8_t g, uint8_t b, bool useLine)
{
    lcd->clear(1,1,1);
    int top = adcCount;
    if (useLine) {
        for (int x0 = 0; x0 < 127; x0++) {
            int y0 = adcValue[(x0 + top) & 0x7f] & 0x7f;
            int x1 = x0 + 1;
            int y1 = adcValue[(x1 + top) & 0x7f] & 0x7f;
            lcd->line(x0, y0, x1, y1, r, g, b);
        }
    } else {
        for (int i = 0; i < 128; i++) {
            lcd->setPixel(i, adcValue[i] & 0x7f, r, g, b);
        }
    }
}

void StartDefaultTask(void const * argument)
{
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

  // Done in usbd_conf.c
  //HAL_NVIC_SetPriority(OTG_FS_IRQn, 15 /* low preempt priority */, 0 /* high sub-priority*/);
  //HAL_NVIC_EnableIRQ(OTG_FS_IRQn);

  pinInitIrq(SW1_PIN, 1);
  pinInitIrq(SW2_PIN, 1);

  HAL_ADC_Start_IT(mb->getAdc1());

  // Init LCD
  lcd->begin();
  lcd->clear(1,1,1);

//  extern char heap_low;
//  extern char heap_top;
//  printf("heap_low: %p\n", &heap_low);
//  printf("heap_top: %p\n", &heap_top);
//  printf("lcd: %p\n", lcd);
//  printf("mb: %p\n", mb);
//  printf("IOCheck: pup:%012llx sh:%012llx ary:%012llx\n", pupCheck, shCheck, shArray);

  int frame = 0;
  char buff[32];
  while (1) {
    toggleLed();
    sprintf(buff, "Frame%04d", frame);
    lcd->putString(buff, 0, 0);
    int tmp = mode % 36;
    if (tmp == 0) {
        int k = frame >> 7;
        lcd->clear(k & 1, k & 2, k & 4);
    } else if (tmp < 3) {
        drawBars(tmp == 2);
    } else if (tmp < 12) {
        drawChecker(tmp == 11 ? (frame&0x7) : (tmp - 3));
    } else if (tmp < 28) {
        drawAdc(tmp&1, tmp&2, tmp&4, tmp >= 20);
    } else {
        lcd->circle(64,64,frame%64, (frame>>6) & 1, (frame>>7) & 1, (frame>>8) & 1);
    }
    frame++;
  }
}

