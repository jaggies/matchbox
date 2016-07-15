/*
 * lcdtest.c
 *
 *  Created on: Jun 20, 2016
 *      Author: jmiller
 */

#include <assert.h>
#include "matchbox.h"
#include "lcd.h"
#include "adc.h"

osThreadId defaultTaskHandle;
static uint8_t mode;

void StartDefaultTask(void const * argument);

static void toggleLed() {
    static uint8_t data;
    writePin(LED_PIN, (data++) & 1);
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
}

int main(void)
{
  MatchBox* mb = new MatchBox();

  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 2048);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  while (1)
      ;

  return 0;
}

static void drawBars(Lcd& lcd, int vertical)
{
    for (int j = 0; j < 128; j++) {
        for (int i = 0; i < 128; i++) {
            uint8_t r, g, b;
            int index = vertical ? j : i;
            lcd.setPixel(i,j, (index >> 4) & 1, (index >> 5) & 1, (index >> 6) & 1);
        }
    }
}

static void drawChecker(Lcd& lcd, int scale)
{
    for (int j = 0; j < 128; j++) {
        for (int i = 0; i < 128; i++) {
            uint8_t pix = ((j>>(scale)) ^ (i>>(scale))) & 1;
            lcd.setPixel(i,j, pix, pix, pix);
        }
    }
}

static void drawAdc(Lcd& lcd, const uint16_t* values, int n, uint8_t r, uint8_t g, uint8_t b, bool useLine)
{
    if (useLine) {
        for (int x0 = 0; x0 < n; x0++) {
            int y0 = values[x0 & 0x7f] & 0x7f;
            int x1 = x0 + 1;
            int y1 = values[x1 & 0x7f] & 0x7f;
            lcd.line(x0, y0, x1, y1, r, g, b);
        }
    } else {
        for (int i = 0; i < n; i++) {
            lcd.setPixel(i, values[i] & 0x7f, r, g, b);
        }
    }
}

static void adcCallback(const uint16_t* values, int n, void* arg) {
    Lcd& lcd = *(Lcd*) arg;
    int tmp = mode % 36;
    if (tmp >= 12 && tmp < 28) {
        static int f = 0;
        // Persist some samples before clearing
        if (!(f++%16)) {
            lcd.clear(1,1,1);
        }
        drawAdc(lcd, values, n, mode&1, mode&2, mode&4, (mode %36) >= 20);
    }
}

void StartDefaultTask(void const * argument)
{
  Spi spi2(Spi::SP2);
  Lcd lcd(spi2);
  Adc adc(Adc::AD1, 128);

  pinInitOutput(LED_PIN, 1);

  pinInitIrq(SW1_PIN, 1);
  HAL_NVIC_SetPriority(EXTI1_IRQn, 15 /* low preempt priority */, 0 /* high sub-priority*/);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  pinInitIrq(SW2_PIN, 1);
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 15 /* low preempt priority */, 0 /* high sub-priority*/);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  // Init LCD
  lcd.begin();
  lcd.clear(1,1,1);

  adc.start(adcCallback, &lcd);

  int frame = 0;
  char buff[32];
  while (1) {
    toggleLed();
    sprintf(buff, "Frame%04d", frame);
    lcd.putString(buff, 0, 0);
    int tmp = mode % 36;
    if (tmp == 0) {
        int k = frame >> 7;
        lcd.clear(k & 1, k & 2, k & 4);
    } else if (tmp < 3) {
        drawBars(lcd, tmp == 2);
    } else if (tmp < 12) {
        drawChecker(lcd, tmp == 11 ? (frame&0x7) : (tmp - 3));
    } else if (tmp < 28) {
        // handled by drawAdc()
    } else {
        lcd.circle(64,64,frame%64, (frame>>6) & 1, (frame>>7) & 1, (frame>>8) & 1);
    }
    frame++;
  }
}
