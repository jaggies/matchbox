/*
 * util.cpp
 *
 *  Created on: Jun 20, 2020
 *      Author: jmiller
 */

#include "util.h"

int32_t toFatTime(const RTC_TimeTypeDef &time_s, const RTC_DateTypeDef& date_s) {
    return (int32_t)(date_s.Date - 80) << 25
         | (int32_t)(date_s.Month + 1) << 21
         | (int32_t)(date_s.Date + 1) << 16
         | (int32_t)(time_s.Hours) << 11
         | (int32_t)(time_s.Minutes) << 5
         | (int32_t)(time_s.Seconds/2);
}



