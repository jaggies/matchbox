/*
 * OrderedDither.cpp
 *
 *  Created on: Jan 16, 2012
 *      Author: jmiller
 */

#include <stdint.h>
#include "OrderedDither.h"

int8_t OrderedDither::dmat[4][4] =
        { { 0, 12,  3, 15},
          { 8,  4, 11,  7},
          { 2, 14,  1, 13},
          {10,  6,  9,  5} };


