/*
 * lcdtest.c
 *
 *  Created on: Jun 20, 2016
 *      Author: jmiller
 */

#include <cstdio>
#include <cassert>
#include <cstring> // memcpy()
#include "matchbox.h"
#include "lcd.h"
#include "adc.h"
#include "pin.h"
#include "util.h"

osThreadId defaultTaskHandle;

void StartDefaultTask(void const * argument);

static void toggleLed() {
    static uint8_t data;
    writePin(LED1, (data++) & 1);
}

int main(void)
{
  MatchBox* mb = new MatchBox(MatchBox::C24MHz);

  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 1, 2048);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */
  while (1)
      ;

  return 0;
}

static void drawBars(Lcd& lcd, bool vertical)
{
    if (vertical) {
        int dx = lcd.getWidth() / 8;
        int x = 0;
        for (int i = 0; i < 8; i++) {
            lcd.setColor((i&4) ? 1 : 0, (i&2) ? 1 : 0, (i&1) ? 1 : 0);
            lcd.rect(x, 0, x + dx, lcd.getHeight() - 1);
            x += dx;
        }
    } else {
        int dy = lcd.getHeight() / 8;
        int y = 0;
        for (int i = 0; i < 8; i++) {
            lcd.setColor((i&4) ? 1 : 0, (i&2) ? 1 : 0, (i&1) ? 1 : 0);
            lcd.rect(0, y, lcd.getWidth() - 1, y + dy);
            y += dy;
        }
    }
}

static void drawChecker(Lcd& lcd, int scale)
{
    int8_t oldclr = -1;
    for (int j = 0; j < 128; j++) {
        lcd.moveTo(0,j);
        for (int i = 0; i < 128; i++) {
            int8_t clr = ((j ^ i) >> scale) & 1;
            if (oldclr != clr) {
                lcd.setColor(clr, clr, clr);
                oldclr = clr;
            }
            lcd.setPixel();
            lcd.incX();
        }
    }
}

static void drawAdc(Lcd& lcd, const uint16_t* values, int n, uint8_t r, uint8_t g, uint8_t b, bool useLine)
{
    if (useLine) {
        lcd.setColor(r, g, b);
        lcd.moveTo(0, values[0] & 0x7f);
        for (int x = 0; x < n; x++) {
            int y = values[x & 0x7f] & 0x7f;
            lcd.lineTo(x, y);
        }
    } else {
        lcd.setColor(r, g, b);
        for (int i = 0; i < n; i++) {
            lcd.setPixel(i, values[i] & 0x7f);
        }
    }
}

static void drawCircles(Lcd& lcd, uint8_t r, uint8_t g, uint8_t b,
        uint8_t br, uint8_t bg, uint8_t bb) {
    lcd.clear(br, bg, bb);
    lcd.setColor(r, g, b);
    for (int i = 0; i < 64; i++) {
        lcd.circle(64, 64, i);
    }
}

static void adcCallback(const uint16_t* values, int n, void* arg) {
    uint16_t* buffer = (uint16_t*) arg;
    memcpy(buffer, values, n * sizeof(uint16_t));
}

void buttonHandler(uint32_t pin, void* data) {
    int &mode = *(int*) data;
    switch (pin) {
        case SW1:
            mode++;
            break;
        default:
            printf("Pin not handled: %d\n", pin);
    }
}

void StartDefaultTask(void const * argument)
{
  uint8_t mode = 0;

  Spi spi2(Spi::SP2, Spi::Config().setOrder(Spi::LSB_FIRST).setClockDiv(Spi::DIV32));
  Lcd lcd(spi2, Lcd::Config().setDoubleBuffered(true));
  Adc adc(Adc::AD1, lcd.getWidth());
  Button b1(SW1, buttonHandler, &mode);
  uint16_t samples[lcd.getWidth()];

  pinInitOutput(LED1, 1);

  int f;
  debug("RCC_HCLK = %d\n", HAL_RCC_GetHCLKFreq());
  debug("HAL_TickFreq = %d\n",
          (f = HAL_GetTickFreq()) == HAL_TICK_FREQ_1KHZ ? 1000
          : f == HAL_TICK_FREQ_100HZ ? 100 : f == HAL_TICK_FREQ_10HZ ? : 0);

  // Init LCD
  lcd.begin();
  lcd.clear(1,1,1);

  adc.start(adcCallback, &samples[0]);

  int frame = 0;
  char buff[32];
  while (1) {
    toggleLed();
    int tmp = mode % 36;
    bool doSwap = true;
    if (tmp == 0) {
        int k = frame;
        lcd.clear(k & 1, k & 2, k & 4);
    } else if (tmp < 3) {
        drawBars(lcd, tmp == 2);
    } else if (tmp < 12) {
        drawChecker(lcd, tmp == 11 ? (frame&0x7) : (tmp - 3));
    } else if (tmp < 28) {
        lcd.clear(1,1,1);
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
