/*
 * LcdInterface.h
 *
 *  Created on: Jan 16, 2012
 *      Author: jmiller
 */

#ifndef LCD_H_
#define LCD_H_

#include <stdint.h>
#include "spi.h"
#include "font.h"

// TODO: Move these to a config file
#define LCD_PEN_PIN PB9
#define LCD_SCLK_PIN PB10
#define LCD_SI_PIN PC3
#define LCD_SCS_PIN PB1
#define LCD_EXTC_PIN PB4
#define LCD_DISP_PIN PB5
#define LCD_SIZE (YRES * LCD_LINE_SIZE) // size of lcd frame in bytes
#define LCD_DEPTH 3 // 3 == color, 1 == mono
#define LCD_XRES 128
#define LCD_YRES 128
#define LCD_LINE_SIZE (LCD_DEPTH*LCD_XRES/8) // size of a line in bytes (data only)

class Lcd {
	public:
        enum FontSize { FONT_SMALL, FONT_MEDIUM, FONT_LARGE };
        struct Config {
                Config() : doubleBuffer(0), en(LCD_PEN_PIN), scs(LCD_SCS_PIN), extc(LCD_EXTC_PIN),
                        disp(LCD_DISP_PIN) { }
                Config& setDoubleBuffered(bool db) { doubleBuffer = db ? 1 : 0; return *this; }
                Config& setPowerEnable(int pin) { en = pin; return *this; }
                Config& setChipSelect(int pin) { scs = pin; return *this; }
                Config& setExtC(int pin) { extc = pin; return *this; }
                Config& setDisplayEnable(int pin) { disp = pin; return *this; }
                int doubleBuffer :1;
                // control pins
                uint8_t en, scs, extc, disp;
        };
        Lcd(Spi& spi, const Config& config = Config());
        ~Lcd();

		void begin(void);
		void setEnabled(bool enabled);
		void setColor(uint8_t r, uint8_t g, uint8_t b);
		void clear(uint8_t r, uint8_t g, uint8_t b);
		void setPixel(uint16_t x, uint16_t y);
		void circle(int16_t x0, int16_t y0, int16_t radius);
		void line(int16_t x0, int16_t y0, int16_t x1, int16_t y1);
		void rect(int16_t x0, int16_t y0, int16_t x1, int16_t y1, bool fill = true);

		// High performance routines
		void moveTo(int16_t x, int16_t y);
		void lineTo(int16_t x, int16_t y);
		void setPixel();
        void incX();
        void decX();
        void incY();
        void decY();

		bool setFont(const char* name);
		inline int putChar(uint8_t c, int x, int y) {
		    return putChar(c, x, y, _fg, _bg);
		}
		int putChar(uint8_t c, int x, int y, const uint8_t* fg, const uint8_t* bg);
		inline void putString(const char *str, int x, int y) {
		    putString(str, x, y, _fg, _bg);
		}
		void putString(const char *str, int x, int y, const uint8_t* fg, const uint8_t* bg);
		void swapBuffers();

		int getHeight() const { return _yres; }
		int getWidth() const { return _xres; }

		// Returns maximum width of all characters
		int getFontWidth() const { return _currentFont ? _currentFont->charWidth : 0; }

		// Returns maximum height of all characters
        int getFontHeight() const { return _currentFont ? _currentFont->charHeight : 0; }

		void refresh();

	private:
		struct Line {
		        uint8_t cmd;
		        uint8_t row;
		        uint8_t data[LCD_LINE_SIZE];
		};

		struct Frame {
		        Frame() : sync1(0), sync2(0) { }
		        Line line[LCD_YRES];
		        uint8_t sync1; // final sync bytes to simplify SPI streaming
		        uint8_t sync2;
		};

		// Draws a horizontal span at the current position
		void span(int16_t dx);

		static void refreshFrameCallback(void* arg);

		void refreshFrame();

		Spi& _spi;
		Config _config;
		uint8_t _frame; // refresh frame count
		const uint16_t _xres, _yres, _depth, _line_size;
		uint8_t _clear;
		const Font* _currentFont;
		Frame* _writeBuffer;
		Frame* _refreshBuffer;
		uint8_t _fg[3];
		uint8_t _bg[3];
		uint8_t _red, _grn, _blu;
		volatile bool _doSwap; // trigger swapBuffer on next frame refresh
        volatile bool _disableRefresh;
        volatile bool _enabled;

        uint32_t _scanIncrement; // in pixels
        uint32_t* _rasterOffset; // in pixels
        int16_t  _rasterX;
        int16_t  _rasterY;
};

#define BITBAND_SRAM_REF 0x20000000
#define BITBAND_SRAM_BASE 0x22000000
#define BITBAND_SRAM(a,b) ((BITBAND_SRAM_BASE + (a-BITBAND_SRAM_REF)*32 + (b*4)))
#define DO_BITBAND

inline void Lcd::setColor(uint8_t r, uint8_t g, uint8_t b) {
    // In bitbanding mode, non-zero means the bit is set
    _red = r ? 1 : 0;
    _grn = g ? 1 : 0;
    _blu = b ? 1 : 0;
}

inline void Lcd::incX() {
    _rasterX++;
    _rasterOffset += _depth;
}

inline void Lcd::decX() {
    _rasterX--;
    _rasterOffset -= _depth;
}

inline void Lcd::incY() {
    _rasterY++;
    _rasterOffset += _scanIncrement;
}

inline void Lcd::decY() {
    _rasterY--;
    _rasterOffset -= _scanIncrement;
}

inline void Lcd::moveTo(int16_t x, int16_t y) {
    // Compute bit address of pixel beyond line cmd and row from Line structure
    _rasterOffset = (uint32_t*) BITBAND_SRAM((int)&_writeBuffer->line[y].data[0], x * _depth);
    _rasterX = x;
    _rasterY = y;
}

inline void Lcd::line(int16_t x0, int16_t y0, int16_t x1, int16_t y1) {
    moveTo(x0, y0);
    lineTo(x1, y1);
}

inline void Lcd::setPixel() {
    _rasterOffset[0] = _red;
    _rasterOffset[1] = _grn;
    _rasterOffset[2] = _blu;
}

inline void Lcd::span(int16_t dx) {
    while (dx-- > 0) {
        setPixel();
        incX();
    }
}

inline void Lcd::setPixel(uint16_t x, uint16_t y)
{
    moveTo(x, y);
    setPixel();
}

#endif /* LCD_H_ */
