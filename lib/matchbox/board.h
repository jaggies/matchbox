/*
 * board.h
 *  Defines all board-specific properties, like switches, hard-wired IO lines, etc.
 *  Created on: Jul 15, 2016
 *      Author: jmiller
 */

#ifndef BOARD_H_
#define BOARD_H_

#define SW1_PIN PC13 // redefined to PC1 later
#define SW2_PIN PA13

#ifdef VERSION_01
#define LED_PIN SW1_PIN // Oops. Wired to PC13
#define POWER_PIN PD2
#else
#define LED_PIN PB2
#define POWER_PIN PA14
#endif

#endif /* BOARD_H_ */
