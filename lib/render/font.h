/*
 * font.h
 *
 *  Created on: Jun 12, 2016
 *      Author: jmiller
 */

#ifndef FONT_H_
#define FONT_H_

struct Font {
    uint8_t columns; // character width, in pixels
    uint8_t rows; // character height, in pixels
    uint8_t charLength; // bytes per character
    uint8_t data[]; // raw character data
};

extern const Font* getFont(int rows, int cols);

#endif /* FONT_H_ */
