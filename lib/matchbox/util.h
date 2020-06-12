/*
 * util.h
 *
 *  Created on: Jul 13, 2016
 *      Author: jmiller
 */

#ifndef UTIL_H_
#define UTIL_H_

#include <stdio.h>

#define Number(a) (sizeof(a) / sizeof(a[0]))

 inline uint8_t bitSwap(uint8_t x)
{
    uint8_t res = 0;
    for (int i = 0; i < 8; i++) {
        res = (res << 1) | (x & 1);
        x >>= 1;
    }
    return res;
}

#if defined(DEBUG)
#define debug(...) printf(__VA_ARGS__)
#define error(...) printf(__VA_ARGS__)
#else
inline void debug(const char* fmt, ...) { }
inline void error(const char* fmt, ...) { }
#endif

#endif /* UTIL_H_ */
