/*
 * lcdtest.c
 *
 *  Created on: Jun 20, 2016
 *      Author: jmiller
 */

#include <assert.h>
#include <string.h> // memcpy()
#include "matchbox.h"
#include "lcd.h"
#include "adc.h"
#include "button.h"

osThreadId defaultTaskHandle;
static uint8_t mode;

void StartDefaultTask(void const * argument);

static void toggleLed() {
    static uint8_t data;
    writePin(LED_PIN, (data++) & 1);
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

static void drawCircles(Lcd& lcd, uint8_t r, uint8_t g, uint8_t b,
        uint8_t br, uint8_t bg, uint8_t bb) {
    lcd.clear(br, bg, bb);
    for (int i = 0; i < 64; i++) {
        lcd.circle(64, 64, i, r, g, b);
    }
}

static void adcCallback(const uint16_t* values, int n, void* arg) {
    uint16_t* buffer = (uint16_t*) arg;
    memcpy(buffer, values, n * sizeof(uint16_t));
}

void buttonHandler(uint32_t pin, void* data) {
    int &mode = *(int*) data;
    switch (pin) {
        case SW2_PIN:
            mode++;
            break;
        default:
            printf("Pin not handled: %d\n", pin);
    }
}

void StartDefaultTask(void const * argument)
{
  Spi spi2(Spi::SP2, Spi::Config().setOrder(Spi::LSB_FIRST));
  Lcd::Config config;
  config.doubleBuffer = 1;
  Lcd lcd(spi2, config);
  Adc adc(Adc::AD1, lcd.getWidth());
  Button sw2(SW2_PIN, buttonHandler, &mode);
  uint16_t samples[lcd.getWidth()];

  pinInitOutput(LED_PIN, 1);

  // Init LCD
  lcd.begin();
  lcd.clear(1,1,1);

  adc.start(adcCallback, &samples[0]);

  int frame = 0;
  char buff[32];
  while (1) {
    toggleLed();
    int tmp = mode % 36;
    bool doSwap = 1;
    if (tmp == 0) {
        int k = frame >> 5;
        lcd.clear(k & 1, k & 2, k & 4);
    } else if (tmp < 3) {
        drawBars(lcd, tmp == 2);
    } else if (tmp < 12) {
        drawChecker(lcd, tmp == 11 ? (frame&0x7) : (tmp - 3));
    } else if (tmp < 28) {
        static int accum = 0;
        if (++accum == 16) {
            lcd.clear(1,1,1);
            accum = 0;
        } else {
            doSwap = false;
        }
        drawAdc(lcd, samples, lcd.getWidth(), mode&1, mode&2, mode&4, (mode %36) >= 20);
    } else {
        drawCircles(lcd, (frame>>5) & 1, (frame>>6) & 1, (frame>>7) & 1,
                tmp & 1, (tmp>>1) & 1, (tmp >> 2) & 1);
    }
    sprintf(buff, "Frame%04d", frame);
    lcd.putString(buff, 0, 0);
    if (doSwap) {
        lcd.swapBuffers();
    }
    frame++;
  }
}
