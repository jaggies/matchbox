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
#define LCD_CHAN 3 // 3 == color, 1 == mono
#define LCD_XRES 128
#define LCD_YRES 128
#define LCD_LINE_SIZE (LCD_CHAN*LCD_XRES/8) // size of a line in bytes (data only)

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
		void clear(uint8_t r, uint8_t g, uint8_t b);
		void setPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b);
		void circle(int x0, int y0, int radius, uint8_t r, uint8_t g, uint8_t b);
		void line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b);
		void rect(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, bool fill = true);

		bool setFont(const char* name);
		inline int putChar(uint8_t c, int x, int y) {
		    return putChar(c, x, y, _fg, _bg);
		}
		int putChar(uint8_t c, int x, int y, const uint8_t* fg, const uint8_t* bg);
		inline void putString(const char *str, int x, int y) {
		    putString(str, x, y, _fg, _bg);
		}
		void putString(const char *str, int x, int y, const uint8_t* fg, const uint8_t* bg);
		void swapBuffers() {
            _doSwap = true; /* handled in refresh */
            while (_doSwap) {
                // wait for ack from interrupt handler
            }
		}

		int getHeight() const { return _yres; }
		int getWidth() const { return _xres; }
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
		        const uint8_t sync1, sync2; // final sync bytes to simplify SPI streaming
		};

		static void refreshFrameCallback(void* arg);
		static void refreshLineCallback(void* arg);
		void refreshFrame();

		Spi& _spi;
		Config _config;
		uint8_t _frame; // refresh frame count
		const uint16_t _xres, _yres, _channels, _line_size;
		uint8_t _clear;
		const Font* _currentFont;
		Frame* _writeBuffer;
		Frame* _refreshBuffer;
		uint8_t _fg[3];
		uint8_t _bg[3];
		volatile bool _doSwap; // trigger swapBuffer on next frame refresh
};

#define BITBAND_SRAM_REF 0x20000000
#define BITBAND_SRAM_BASE 0x22000000
#define BITBAND_SRAM(a,b) ((BITBAND_SRAM_BASE + (a-BITBAND_SRAM_REF)*32 + (b*4)))
#define DO_BITBAND

inline void Lcd::setPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b)
{
#ifdef DO_BITBAND
    uint32_t pixaddr = x * _channels;
    uint32_t* pixel = (uint32_t*)BITBAND_SRAM((int)&_writeBuffer->line[y].data[0], pixaddr);
    *pixel++ = r ? 1 : 0;
    *pixel++ = g ? 1 : 0;
    *pixel = b ? 1 : 0;
#else
    uint16_t rbitaddr = x * _channels + 0;
    uint16_t rbyteaddr = rbitaddr / 8;
    uint16_t rbit = rbitaddr % 8;
    _frameBuffer[y].data[rbyteaddr] &= ~(1 << rbit);
    _frameBuffer[y].data[rbyteaddr] |= r ? (1 << rbit) : 0;

    uint16_t gbitaddr = x * _channels + 1;
    uint16_t gbyteaddr = gbitaddr / 8;
    uint16_t gbit = gbitaddr % 8;
    _frameBuffer[y].data[gbyteaddr] &= ~(1 << gbit);
    _frameBuffer[y].data[gbyteaddr] |= g ? (1 << gbit) : 0;

    uint16_t bbitaddr = x * _channels + 2;
    uint16_t bbyteaddr = bbitaddr / 8;
    uint16_t bbit = bbitaddr % 8;
    _frameBuffer[y].data[bbyteaddr] &= ~(1 << bbit);
    _frameBuffer[y].data[bbyteaddr] |= b ? (1 << bbit) : 0;
#endif
}

#endif /* LCD_H_ */
