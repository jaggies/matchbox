/*
 * triangle.cpp
 *
 *  Created on: Jan 26, 2012
 *      Author: jmiller
 */

#include <stdint.h>
#include <algorithm> // min/max
#include "gpio.h"
#include "OrderedDither.h"
#include "Interpolator.h"
#include "triangle.h"
#include "config.h"

#ifdef ENABLE_DITHERING
const int shift = 0;
#else
const int shift = INTERPOLATOR_UNIT;
#endif

template <class T>
void swap(T& a, T& b) {
	T tmp = a;
	a = b;
	b = tmp;
}

class Edge {
	public:
		Edge(const Vertex& from, const Vertex& to, int delta, int offset = 0) :
			x(from.x, to.x, delta, offset),
			r(from.r, to.r, delta, offset),
			g(from.g, to.g, delta, offset),
			b(from.b, to.b, delta, offset) { }

		inline void start(const Vertex& from, const Vertex& to, int dy, int offset = 0) {
			x.start(from.x, to.x, dy, offset);
			r.start(from.r, to.r, dy, offset);
			g.start(from.g, to.g, dy, offset);
			b.start(from.b, to.b, dy, offset);
		}

		inline void advance(int n) {
			x.advance(n);
			r.advance(n);
			g.advance(n);
			b.advance(n);
		}

		Interpolator<int, INTERPOLATION_FRACTION, 0> x;
		Interpolator<int, INTERPOLATION_FRACTION, 0> r;
		Interpolator<int, INTERPOLATION_FRACTION, 0> g;
		Interpolator<int, INTERPOLATION_FRACTION, 0> b;
};

inline static void drawTrapezoid(Lcd& lcd, Edge& edge1, Edge& edge2, int ymin, int ymax)
{
	const int bottom = 0;
	const int top = lcd.getWidth();
	const int leftLimit = 0;
	const int rightLimit = lcd.getHeight();
	if (ymin < bottom) {
		const int offset = bottom - ymin;
		edge1.advance(offset);
		edge2.advance(offset);
		ymin = bottom;
	}
	ymax = std::min(top, ymax);
	Edge *left = &edge1;
	Edge *right = &edge2;
	for (int y = ymin; y < ymax; y++) {
		int start = left->x.nextValue();
		int stop = right->x.nextValue();
		if (start > stop) {
			swap(start, stop);
			swap(left, right);
		}
		const int dist = stop - start;
		const int x1 = std::max(leftLimit, start);
		const int x2 = std::min(rightLimit, stop);
		const int offset = x1 - start;
		const int dx = x2 - x1;
		Interpolator<int, INTERPOLATION_FRACTION, shift>
			red(left->r.nextValue(), right->r.nextValue(), dist, offset),
			grn(left->g.nextValue(), right->g.nextValue(), dist, offset),
			blu(left->b.nextValue(), right->b.nextValue(), dist, offset);
		for (int x = x1; x <= x2; x++) {
			lcd.setPixel(x, y, red.nextValue(), grn.nextValue(), blu.nextValue());
		}
	}
}

void drawTriangle(Lcd& lcd, const Vertex* v0, const Vertex* v1, const Vertex* v2)
{
	// Sort points by Y
	if (v0->y > v1->y) swap(v0, v1);
	if (v1->y > v2->y) {
		swap(v1, v2);
		if (v0->y > v1->y) swap(v0, v1);
	}

	// Upper part of triangle
	const bool downFacing = v0->y == v1->y;
	if (!downFacing) {
		Edge edge1(*v0, *v1, (v1->y - v0->y));
		Edge edge2(*v0, *v2, (v2->y - v0->y));
		drawTrapezoid(lcd, edge1, edge2, v0->y, std::min(v1->y, v2->y));
	}

	// Lower part of triangle
	const bool upFacing = v1->y == v2->y;
	if (!upFacing) {
		Edge edge1(*v0, *v2, v2->y - v0->y, std::max(0, v1->y - v0->y));
		Edge edge2(*v1, *v2, v2->y - v1->y, std::max(0, v0->y - v1->y));
		drawTrapezoid(lcd, edge1, edge2, std::max(v0->y, v1->y), v2->y);
	}
}
