/*
 * Lcd.cpp
 *
 *  Created on: Jan 16, 2012
 *      Author: jmiller
 */

#include <algorithm>
#include <cstring>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include "font.h"
#include "gpio.h"
#include "lcd.h"
#include "util.h"

//#define SOFT_SPI
volatile int dly;
void delay(int n)
{
    dly = n;
    while (dly--);
}

static inline uint8_t bitSwap(uint8_t x)
{
    uint8_t res = 0;
    for (int i = 0; i < 8; i++) {
        res = (res << 1) | (x & 1);
        x >>= 1;
    }
    return res;
}

Lcd::Lcd(SPI_HandleTypeDef& spi, uint8_t en, uint8_t sclk, uint8_t si, uint8_t scs, uint8_t extc,
        uint8_t disp) : _spi(spi), _en(en), _sclk(sclk), _si(si), _scs(scs), _extc(extc),
        _disp(disp), _clear(1), _row(0), _frame(0), _dither(8, 1), _currentFont(0),
        _xres(LCD_XRES), _yres(LCD_YRES), _channels(LCD_CHAN), _line_size(LCD_XRES*LCD_CHAN/8)
{
    _currentFont = getFont("roboto_bold_10");
}

void Lcd::begin() {
#ifdef SOFT_SPI
    uint8_t pins[] = {_en, _sclk, _si, _scs, _extc, _disp };
    uint8_t defaults[] = { 1, 0, 0, 0, 0, 1 };
#else
    // Skip sclk and si since SPI initializes them for alternate functionality (spi)
    uint8_t pins[] = {_en, _scs, _extc, _disp };
    uint8_t defaults[] = { 1, 0, 0, 1 };
#endif
    for (int i = 0; i < Number(pins); i++) {
        pinInitOutput(pins[i], defaults[i]);
    }
    bzero(&_frameBuffer, sizeof(_frameBuffer));
}

void Lcd::sendByte(uint8_t b) {
#ifdef SOFT_SPI
    for (int i = 0; i < 8; i++) {
        writePin(_sclk, 0);
        writePin(_si, (b >> i) & 1);
        writePin(_sclk, 1);
    }
    writePin(_sclk, 0);
#else
    HAL_StatusTypeDef status = HAL_SPI_Transmit(&_spi, &b, 1, 1000);
    if (status != HAL_OK) {
        printf("sendByte(): error = %d\n", status);
    }
#endif
}

void Lcd::sendBytes(uint8_t* data, uint16_t count) {
#ifdef SOFT_SPI
    for (uint16_t i = 0; i < count; i++) {
        sendByte(data[i]);
    }
#else
//    HAL_StatusTypeDef status = HAL_SPI_Transmit(&_spi, data, count, 1000);
    HAL_StatusTypeDef status = HAL_SPI_Transmit_IT(&_spi, data, count);
    if (status != HAL_OK) {
        printf("sendBytes(): error = %d\n", status);
    }
#endif
}

// Single line format : SCS CMD ROW <data0..n> 0 0 SCS#
// Multi-line format : SCS CMD ROW <data0..n> IGNORED ROW <data0..127> ... SCS#
void Lcd::refreshLine(int row) {
    sendByte(_frameBuffer[row].cmd);
    sendByte(_frameBuffer[row].row); // first row is 1, not 0 and bitswapped :/
    sendBytes(_frameBuffer[row].data, _line_size);
}

void Lcd::refresh() {
    refreshFrameSpi();
}

// Manually update the display
void Lcd::refreshFrame() {
    writePin(_scs, 1);
    for (int i = 0; i < _yres; i++) {
        _frameBuffer[i].cmd = bitSwap(0x80 | (_frame ? 0x40:0) | (_clear ? 0x20 : 0));
        _frameBuffer[i].row = i + 1;
        refreshLine(i);
    }
    writePin(_scs, 0);
    writePin(_extc, (_frame++) & 0x01);
    _clear = 0;
}

void Lcd::refreshFrameSpi() {
    // Toggle common driver once per frame
    writePin(_extc, (_frame++) & 0x01);

    writePin(_scs, 0); // cs disabled
    uint8_t cmd = bitSwap(0x80 | (_frame ? 0x40:0) | (_clear ? 0x20 : 0));
    for (int i = 0; i < _yres; i++) {
        _frameBuffer[i].cmd = cmd;
        _frameBuffer[i].row = i + 1;
    }
    writePin(_scs, 1); // cs enabled
    HAL_StatusTypeDef status = HAL_SPI_Transmit_IT(&_spi, (uint8_t*)&_frameBuffer[0],
            sizeof(_frameBuffer));
    assert(HAL_OK == status);
    _clear = 0;
}

void Lcd::refreshLineSpi() {
    static uint8_t clear = 0; // TODO
    HAL_StatusTypeDef status;
    if (_row == 0) {
        writePin(_scs, 1);
    } else if (_row == 128) {
        uint8_t zero = 0;
        status = HAL_SPI_Transmit(&_spi, &zero, 1, 1000);
        assert(HAL_OK == status);
        _row = 0;
        _frame++;
        writePin(_scs, 0);
        delay(100);
        writePin(_scs, 1);
    }

    // Send one line
    _frameBuffer[_row].cmd = bitSwap(0x80 | (_frame ? 0x40:0) | (clear ? 0x20 : 0));
    _frameBuffer[_row].row = _row + 1; // first row starts at 1
    status = HAL_SPI_Transmit_IT(&_spi, (uint8_t*) &_frameBuffer[_row], _line_size);
    assert(HAL_OK == status);
    _row++;
}

void
Lcd::clear(uint8_t r, uint8_t g, uint8_t b) {
    uint8_t p[3];
    // TODO: handle monochrome displays and dithering...
    uint32_t* pixel = (uint32_t*)BITBAND_SRAM((int) &p[0], 0); // addr of first pixel
    for (int i = 0; i < _channels * 8; i+=_channels) {
        *pixel++ = r ? 1 : 0;
        *pixel++ = g ? 1 : 0;
        *pixel++ = b ? 1 : 0;
    }
    for (int j = 0; j < _yres; j++) {
        uint8_t* pixels = &_frameBuffer[j].data[0];
        for (int i = 0; i < _line_size/3; i++) {
            *pixels++ = p[0];
            *pixels++ = p[1];
            *pixels++ = p[2];
        }
    }
}

void Lcd::line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b) {
	int dx = abs(x1-x0);
	int dy = abs(y1-y0);
	const int stepX = x0 < x1 ? 1 : -1;
	const int stepY = y0 < y1 ? 1 : -1;
	int err = dx - dy;
    uint32_t pixaddr = x0 * _channels;
	while (x0 != x1 && y0 != y1) {
		setPixel(x0, y0, r, g, b); // TODO: optimize with bit banding
		int e2 = err << 1;
		if (e2 <  dx) {
		   err += dx;
		   y0 += stepY;
		}
		if (e2 > -dy) {
		   err -= dy;
		   x0 += stepX;
		}
	}
}

void Lcd::circle(int x0, int y0, int radius, uint8_t r, uint8_t g, uint8_t b)
{
	int f = 1 - radius;
	int ddF_x = 0;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;
	setPixel(x0, y0 + radius, r, g, b);
	setPixel(x0, y0 - radius, r, g, b);
	setPixel(x0 + radius, y0, r, g, b);
	setPixel(x0 - radius, y0, r, g, b);
	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x + 1;
		setPixel(x0 + x, y0 + y, r, g, b);
		setPixel(x0 - x, y0 + y, r, g, b);
		setPixel(x0 + x, y0 - y, r, g, b);
		setPixel(x0 - x, y0 - y, r, g, b);
		setPixel(x0 + y, y0 + x, r, g, b);
		setPixel(x0 - y, y0 + x, r, g, b);
		setPixel(x0 + y, y0 - x, r, g, b);
		setPixel(x0 - y, y0 - x, r, g, b);
	}
}

void Lcd::rect(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, bool fill)
{
	if (fill) {
		const int xmin = std::min(x0, x1);
		const int xmax = std::max(x0, x1);
		const int ymin = std::min(y0, y1);
		const int ymax = std::max(y0, y1);
		for (int j = ymin; j < ymax; j++) {
		    for (int i = xmin; i < xmax; i++) {
		        setPixel(i, j, r, g, b);
		    }
		}
	} else {
		line(x0, y0, x1, y0, r, g, b);
		line(x0, y1, x1, y1, r, g, b);
		line(x0, y0, x0, y1, r, g, g);
		line(x1, y0, x1, y1, r, g, b);
	}
}

int Lcd::putChar(uint8_t c, int x, int y) {
    static const uint8_t fColor[] = {0, 0, 0}; // TODO: allow these to be set
    static const uint8_t bColor[] = {0xff, 0xff, 0xff};

    if (!_currentFont) return 0;

    // Find character to print. TODO: use binary search
    const CharData* charData = 0;
    for (int i = 0; i < _currentFont->charCount; i++) {
        if (c == _currentFont->charData[i].ch) {
            charData = &_currentFont->charData[i];
            break;
        }
    }

    if (!charData) {
        return 0; // character not found
    }

	for (int j = 0; j < charData->height; j++) {
		for (int i = 0; i < charData->width; i++) {
		    const int bitAddr = (charData->y + j) * _currentFont->width + (charData->x + i);
		    const int byteAddr = bitAddr / 8;
		    const int bitMask = 1 << (bitAddr % 8);
			const uint8_t* color = (_currentFont->bitmap[byteAddr] & bitMask) ? fColor : bColor;
			setPixel(i + x, j + y, color[0], color[1], color[2]);
		}
	}
	return charData->width;
}

void Lcd::putString(const char *str, int x, int y) {
	while (char ch = *str++) {
		x += putChar(ch, x, y);
	}
}

