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

// LCD assigned to SPI2
#define LCD_PEN_PIN 	PB9
#define LCD_SCLK_PIN	SPI2_SCK 
#define LCD_SI_PIN		SPI2_MOSI
#define LCD_SCS_PIN 	PB1
#define LCD_EXTC_PIN 	PB4
#define LCD_DISP_PIN 	PB5

// Bluetooth assigned to SPI3
#define NRF8K_SCK SPI3_SCK 
#define NRF8K_MISO SPI3_MISO
#define NRF8K_MOSI SPI3_MOSI
#define NRF8K_REQN SPI3_NSS

// RTC
#define RTC_ASYNCH_PREDIV       0x7fU // 128
#define RTC_SYNCH_PREDIV        0x00ffU // * 256 = 32768

#endif /* BOARD_H_ */
