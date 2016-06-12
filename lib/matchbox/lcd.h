/*
 * LcdInterface.h
 *
 *  Created on: Jan 16, 2012
 *      Author: jmiller
 */

#ifndef LCD_H_
#define LCD_H_

#include <stdint.h>
#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_spi.h"
#include "OrderedDither.h"
#include "font.h"

class Lcd {
    // TODO: make these template parameters
    static const int xres = 128;
    static const int yres = 128;
    static const int channels = 3; // RGB LCD
    static const int line_size = channels * xres / 8;
	public:
        enum FontSize { FONT_SMALL, FONT_MEDIUM, FONT_LARGE };
        Lcd(SPI_HandleTypeDef& spi, uint8_t en = PB9, uint8_t sclk = PB10,
                uint8_t si = PC3, uint8_t scs = PB1, uint8_t extc = PB4, uint8_t disp = PB5)
	            : _spi(spi), _en(en), _sclk(sclk), _si(si), _scs(scs), _extc(extc), _disp(disp),
	              _clear(1), _row(0), _frame(0), _dither(8, 1), _currentFont(0) {
            _instance = this;
        }
		void begin(void);
		void clear(uint8_t r, uint8_t g, uint8_t b);
		void setPixel(uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b);
		void circle(int x0, int y0, int radius, uint8_t r, uint8_t g, uint8_t b);
		void line(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b);
		void rect(int x0, int y0, int x1, int y1, uint8_t r, uint8_t g, uint8_t b, bool fill = true);
		int putChar(uint8_t c, int x, int y);
		void putString(const char *str, int x, int y);

		int rows() const { return yres; }
		int cols() const { return xres; }
		void flush() { }
		void refresh();

	private:
		void sendByte(uint8_t b);
		void sendBytes(uint8_t* data, uint16_t count);
		void sendLine(uint8_t* buff, int row, int frame, int clear);

		// SPI bus
		SPI_HandleTypeDef& _spi;

		// IRQ/DMA handler data
		static Lcd* _instance;
		uint8_t _frame; // refresh frame count
		uint8_t _row; // refresh rown count
		static void refreshLineIrq();
		void refreshLine();

		// Inlining these methods really speed things up but cost lots of space...
		int dither(int x, int y, int r, int g, int b);
		void color(int color);
		uint8_t _en, _sclk, _si, _scs, _extc, _disp, _clear;
		OrderedDither _dither;
		Font* const _currentFont;
		uint8_t buffer[yres * line_size];
};

#endif /* LCD_H_ */
