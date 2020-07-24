/*
 * font.h
 *
 *  Created on: June 22, 2016
 *      Author: jmiller
 */

#ifndef FONT_H
#define FONT_H

typedef struct _CharData {
	uint8_t ch;
	uint16_t x, y; // position in the bitmap
	uint8_t width, height;
} CharData;

typedef struct _Font {
	const char* name; // name of font
	const uint16_t width; // bitmap width, in pixels
	const uint16_t height; // bitmap height, in pixels
	const uint8_t  charWidth; // max width of all characters in pixels
	const uint8_t  charHeight; // max height of all characters in pixels
	const uint16_t charCount; // number of characters in font
	const CharData *charData;
	const uint8_t* bitmap; // raw character data as MxN bitmap
} Font;

extern const Font* getFont(const char* name);

#endif // FONT_H
