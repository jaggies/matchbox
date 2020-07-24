/*
 * board.h
 *  Defines all board-specific properties not defined in the pinmap from STMCubeMX.
 * 
 *  Created on: Jul 15, 2016
 *      Author: jmiller
 */

#ifndef BOARD_H_
#define BOARD_H_

#include "pinio.h"
#include "pinmap.h" // MCU-dependent mapping of pins generated from STM32CubeMX file

#define LCD_PEN_PIN 	PB9
#define LCD_SCLK_PIN	SPI2_SCK 
#define LCD_SI_PIN		SPI2_MOSI
#define LCD_SCS_PIN 	PB1
#define LCD_EXTC_PIN 	PB4
#define LCD_DISP_PIN 	PB5
#define RTC_ASYNCH_PREDIV	0U
#define RTC_SYNCH_PREDIV	31U 

#endif /* BOARD_H_ */
