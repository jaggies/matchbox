/*
 * OrderedDither.h
 *
 *  Created on: Jan 16, 2012
 *      Author: jmiller
 */

#ifndef ORDEREDDITHER_H_
#define ORDEREDDITHER_H_
#include <stdint.h>

class OrderedDither {
    public:
        OrderedDither(int16_t inBits, int16_t outBits) : _inBits(inBits) {
        	const int bits = inBits - outBits;
        	_loMask = (1 << bits) - 1;
        	_scaleFactor = ((1 << _inBits) - 1) & (~_loMask);
        	_shift = _inBits - outBits;
        }
        inline int16_t dither(int16_t x, int16_t y, int16_t value) const {
        	// TODO: Make this more general; this is optimized for 4 bits (16 levels)
        	value = (value * _scaleFactor) >> _inBits;
        	const int baseValue = value >> _shift;
        	const int error = value & _loMask;
            return baseValue + (error > dmat[y&3][x&3]);
        }
    private:
        int16_t _inBits;
        int16_t _loMask;
        int16_t _scaleFactor;
        int16_t _shift;
        static int8_t dmat[4][4];
};

#endif /* ORDEREDDITHER_H_ */
