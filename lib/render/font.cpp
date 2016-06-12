/*
 * font.cpp
 *
 *  Created on: June 12, 2016
 *      Author: jmiller
 */

#include <stdint.h>
#include "font.h"

#define PROGMEM
const Font Font6x8 PROGMEM = { 0 }; // TODO
const Font Font8x8 PROGMEM = { 0 }; // TODO
const Font * const fontTable[] = { &Font6x8, &Font8x8 };

const Font* getFont(int rows, int cols)
{
    for (int i = 0; i < sizeof(fontTable) / sizeof(fontTable[0]); i++) {
        if (fontTable[i]->rows == rows && fontTable[i]->columns == cols) {
            return fontTable[i];
        }
    }
    return 0;
}
