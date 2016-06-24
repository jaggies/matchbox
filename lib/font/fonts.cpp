/*
 * font.h
 *
 *  Created on: June 22, 2016
 *      Author: jmiller
 */

#include <stdint.h>
#include <string.h>
#include "font.h"
#include "roboto_bold_10.h"
#include "roboto_bold_14.h"

const Font* fonts[] = { &roboto_bold_10, &roboto_bold_14 };

const Font* getFont(const char* name)
{
	for (int i = 0; i < sizeof(fonts) / sizeof(fonts[0]); i++) {
		if (0 == strcmp(fonts[i]->name, name)) {
			return fonts[i];
		}
	}
	return (Font*) 0;
}
