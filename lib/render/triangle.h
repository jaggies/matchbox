/*
 * triangle.h
 *
 *  Created on: Jan 26, 2012
 *      Author: jmiller
 */

#ifndef TRIANGLE_H_
#define TRIANGLE_H_

#include <stdint.h>
#include "lcd.h"

struct Vertex {
	Vertex(int _x, int _y, int _z, int _r, int _g, int _b)
		: x(_x), y(_y), z(_z), r(_r), g(_g), b(_b) { }
	int x, y, z;
	uint8_t r;
	uint8_t g;
	uint8_t b;
};

void drawTriangle(Lcd& lcd, const Vertex* v0, const Vertex* v1, const Vertex* v2);

#endif /* TRIANGLE_H_ */
