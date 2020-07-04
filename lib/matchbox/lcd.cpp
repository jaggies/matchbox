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
#include "cmsis_os.h" // osThreadYield()

#define BLE_PRESENT

Lcd::Lcd(Spi& spi, const Config& config) : _spi(spi), _config(config),
        _clear(1), _frame(0), _currentFont(getFont("roboto_bold_10")),
        _xres(LCD_XRES), _yres(LCD_YRES), _depth(LCD_DEPTH), _line_size(LCD_XRES*LCD_DEPTH/8),
        _frontBuffer(new Frame()), _doSwap(true), _disableRefresh(false), _enabled(false),
        _red(0), _grn(0), _blu(0), _drawSelection(DRAW_BACK), _scanIncrement(sizeof(Line) * 8),
        _rasterX(0), _rasterY(0), _rasterOffset(0)
{
    bool doubleBuffer = config.doubleBuffer;
    _backBuffer = doubleBuffer ? new Frame() : _frontBuffer;
    _drawBuffer = _backBuffer;

    // Ensure the LCD is properly updated when initialized
    memset(_backBuffer, 0xa5, sizeof(Lcd::Frame));
    refreshLineBuffers(_backBuffer);
    if (_backBuffer != _frontBuffer) {
        memset(_frontBuffer, 0xa5, sizeof(Lcd::Frame));
        refreshLineBuffers(_frontBuffer);
    }

    moveTo(0,0); // force _rasterOffset calculation
    memset(_fg, 0, sizeof(_fg));
    memset(_bg, 0xff, sizeof(_bg));
}

Lcd::~Lcd() {
    if (_backBuffer != _frontBuffer) {
        delete _backBuffer; // double buffered
    }
    delete _frontBuffer;
}

void Lcd::begin() {
    // Skip sclk and si since SPI initializes them for alternate functionality (spi)
#ifdef BLE_PRESENT // BLE uses extc for MISO. Use software refresh
    uint8_t pins[] = {_config.en, _config.scs };
    uint8_t defaults[] = { 1, 0 };
#else
    uint8_t pins[] = {_config.en, _config.scs, _config.extc, _config.disp };
    uint8_t defaults[] = { 1, 0, 0, 1 };
#endif
    for (int i = 0; i < Number(pins); i++) {
        pinInitOutput(pins[i], defaults[i]);
    }
    bzero(_frontBuffer, sizeof(*_frontBuffer));
    setEnabled(true);
}

void Lcd::setEnabled(bool enabled) {
    if (enabled != _enabled) {
        _enabled = enabled;
        if (enabled) {
            refreshFrame();
        } else {
            _disableRefresh = true;
        }
        writePin(_config.en, enabled ? 1 : 0);
    }
}

// Called when SPI finishes transfering the current frame
void Lcd::refreshFrameCallback(void* arg) {
    Lcd* thisPtr = (Lcd*) arg;
    thisPtr->refreshFrame(); // frame done, do it again! TODO: Maybe limit this to 30Hz
}

// Single line format : SCS CMD ROW <data0..n> 0 0 SCS#
// Multi-line format : SCS CMD ROW <data0..n> IGNORED ROW <data0..127> ... SCS#
void Lcd::refreshFrame() {
    if (_disableRefresh) {
        _disableRefresh = false;
        return; // don't start another refresh cycle
    }
    if (_doSwap) {
        std::swap(_frontBuffer, _backBuffer);
        moveTo(_rasterX, _rasterY); // force address calculation
        _drawBuffer = _drawSelection == DRAW_FRONT ? _frontBuffer : _backBuffer;
        _doSwap = false;
    }
#ifndef BLE_PRESENT
    writePin(_config.extc, (_frame) & 0x01); // Toggle common driver once per frame
#else
    // TODO: ensure _clear is always in the correct state
    _frontBuffer->line[0].cmd = bitSwap(0x80 | (_frame ? 0x40:0) | (_clear ? 0x20 : 0));
#endif

    writePin(_config.scs, 0); // cs disabled
    writePin(_config.scs, 1); // cs enabled
    Spi::Status status = _spi.transmit((uint8_t*)_frontBuffer, sizeof(*_frontBuffer),
            refreshFrameCallback, this);
    if (Spi::OK != status) {
        error("Failed to refresh with status=%d\n", status);
    }
    _clear = 0;
    _frame++; // Toggle common driver once per frame
}

// Writes all SPI command content. This should be done after drawing in case
// cmd/row data gets clobbered, which will cause the LCD to misbehave.
void Lcd::refreshLineBuffers(Frame* frame) {
    const uint8_t cmd = bitSwap(0x80 | (_frame ? 0x40:0) | (_clear ? 0x20 : 0));
    for (int i = 0; i < _yres; i++) {
        frame->line[i].cmd = cmd;
        frame->line[i].row = i + 1;
    }
    frame->sync1 = _drawBuffer->sync2 = 0; // in case these get clobbered
}

void
Lcd::clear(uint8_t r, uint8_t g, uint8_t b) {
    // Fill up local pixel byte array
    uint32_t* clear = (uint32_t*)&_drawBuffer->line[0].data[0];
    uint32_t* pixel = (uint32_t*)BITBAND_SRAM((int) clear, 0);
    r = r ? 1 : 0; g = g ? 1 : 0; b = b ? 1 : 0;
    for (int i = 0; i < _depth * 8 * 4; i+=_depth) {
        *pixel++ = r;
        *pixel++ = g;
        *pixel++ = b;
    }
    // Use int array to blast pixels
    for (int j = 0; j < _yres; j++) {
        uint32_t* pixels = (uint32_t*) &_drawBuffer->line[j].data[0];
        for (int i = 0; i < _line_size/_depth/sizeof(clear[0]); i++) {
            *pixels++ = clear[0];
            *pixels++ = clear[1];
            *pixels++ = clear[2];
        }
    }

    if (!_config.doubleBuffer) {
        refreshLineBuffers(_drawBuffer);
    }
}

void Lcd::lineTo(int16_t x1, int16_t y1) {
	int dx = abs(x1-_rasterX);
	int dy = abs(y1-_rasterY);
	const int16_t incX = _rasterX < x1 ? 1 : -1;
	const int16_t incY = _rasterY < y1 ? 1 : -1;
	const int32_t stepX = _rasterX < x1 ? _depth : -_depth;
	const int32_t stepY = _rasterY < y1 ? _scanIncrement : -_scanIncrement;
	int err = dx - dy;
	int pixels = std::max(dx, dy);
	while (pixels--) {
		setPixel();
		int e2 = err << 1;
		if (e2 <  dx) {
		   err += dx;
		   _rasterOffset += stepY;
		}
		if (e2 > -dy) {
		   err -= dy;
		   _rasterOffset += stepX;
		}
	}
	_rasterX = x1;
	_rasterY = y1;
}

void Lcd::circle(int16_t x0, int16_t y0, int16_t radius)
{
	int f = 1 - radius;
	int ddF_x = 0;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;
	setPixel(x0, y0 + radius);
	setPixel(x0, y0 - radius);
	setPixel(x0 + radius, y0);
	setPixel(x0 - radius, y0);
	while (x < y) {
		if (f >= 0) {
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		x++;
		ddF_x += 2;
		f += ddF_x + 1;
		setPixel(x0 + x, y0 + y);
		setPixel(x0 - x, y0 + y);
		setPixel(x0 + x, y0 - y);
		setPixel(x0 - x, y0 - y);
		setPixel(x0 + y, y0 + x);
		setPixel(x0 - y, y0 + x);
		setPixel(x0 + y, y0 - x);
		setPixel(x0 - y, y0 - x);
	}
}

void Lcd::rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool fill)
{
	if (fill) {
	    const int dx = abs(x1 - x0);
		const int xmin = std::min(x0, x1);
		const int ymin = std::min(y0, y1);
		const int ymax = std::max(y0, y1);
		for (int j = ymin; j < ymax; j++) {
		    moveTo(xmin, j);
		    span(dx);
		}
	} else {
		line(x0, y0, x1, y0);
		line(x0, y1, x1, y1);
		line(x0, y0, x0, y1);
		line(x1, y0, x1, y1);
	}
}

bool Lcd::setFont(const char* name) {
    const Font* font = getFont(name);
    if (font) {
        _currentFont = font;
        return true;
    }
    return false;
}

int Lcd::putChar(uint8_t c, const uint8_t* fg, const uint8_t* bg) {

    if (!_currentFont) return 0;

    if (!isprint(c)) {
        if (c == '\n') {
            moveTo(0, _rasterY + getFontHeight());
        }
        return 0;
    }
    // Find character to print.
    int first = 0, last = _currentFont->charCount;
    int mid;
    while (first <= last) {
        mid = (first + last) / 2;
        const uint8_t chr = _currentFont->charData[mid].ch;
        if (c < chr) {
            last = mid-1;   // Search left half
        } else if (c > chr) {
            first = mid+1;  // Search right half
        } else {
            break;
        }
    }

    if (first > last) {
        return 0; // character not found
    }
    const CharData* charData = &_currentFont->charData[mid];
    const int charWidthBits = charData->width * _depth;
    setColor(fg[0], fg[1], fg[2]);
    for (int j = 0; j < charData->height; j++) {
        int bitAddr = (charData->y + j) * _currentFont->width + charData->x;
        for (int i = 0; i < charData->width; i++) {
            const int byteAddr = bitAddr / 8;
            const int bitMask = 1 << (bitAddr % 8);
            if (!(_currentFont->bitmap[byteAddr] & bitMask)) {
                setPixel();
            }
            incX();
            bitAddr++;
        }
        _rasterX -= charData->width; // in pixels
        _rasterOffset -= charWidthBits; // reset to start of character
        incY();
    }
    // When we leave this routine, raster should point to the top left of next character
    _rasterOffset -= charData->height * _scanIncrement;
    _rasterOffset += charWidthBits;
    _rasterY -= charData->height;
    return charData->width;
}

void Lcd::putString(const char *str, const uint8_t* fg, const uint8_t *bg) {
	while (char ch = *str++) {
	    putChar(ch, fg, bg) * _depth;
	}
}

void Lcd::swapBuffers() {
    _doSwap = _config.doubleBuffer; /* reset in refresh */

    if (_config.doubleBuffer) {
        refreshLineBuffers(_drawBuffer);
    }

    while (_doSwap) {
        // wait for ack from interrupt handler
        osThreadYield();
    }
}

