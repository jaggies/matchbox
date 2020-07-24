/*
 * cpu_v01.h
 *
 *  Created on: Jun 25, 2020
 *      Author: jmiller
 */

#ifndef LIB_MATCHBOX_CPU_V01_H_
#define LIB_MATCHBOX_CPU_V01_H_

// GPIO pins used on v0.1 CPU board
#define SW1_PIN PC1 // rewired to PC1 from PC13
#define SW2_PIN PA13
#define POWER_PIN PD2
#define LED_PIN SW1_PIN // Wired to PC13 on unmodified board
#define SPI3_SCK PB3
#define SPI3_MISO PB4
#define SPI3_MOSI PB5

#endif /* LIB_MATCHBOX_CPU_V01_H_ */
