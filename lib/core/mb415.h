/*
 * mb415.h
 *
 *  Created on: Jun 25, 2020
 *      Author: jmiller
 *
 *  PIN definitions for matchbox based on stm32f415 part.
 */

#ifndef LIB_MATCHBOX_MB415_H_
#define LIB_MATCHBOX_MB415_H_

// ******** GPIOA ********
#define UNUSED_PA0 PA0
#define UNUSED_PA1 PA1
#define UNUSED_PA2 PA2
#define UNUSED_PA3 PA3
#define SPI1_SS PA4
#define SPI1_SCK PA5
#define SPI1_MISO PA6
#define SPI1_MOSI PA7
#define OTG_FS_ID
#define OTG_FS_DM
#define OTG_FS_DP
#define SWDIO PA13
#define SWCLK PA14
#define SPI3_SS PA15

// ******** GPIOB ********
#define OTG_D1 PB0
#define OTG_D2 PB1
#define BOOT1 PB2
#define SPI3_SCK PB3
#define SPI3_MISO PB4
#define SPI3_MOSI PB5
#define I2C1_SCL PB6
#define I2C1_SDA PB7
#define USART1_TX PB6
#define USART1_RX PB7
#define SDIO_D4 PB8
#define SDIO_D5 PB9
#define I2C2_SCL PB10
#define I2C2_SDA PB11
#define USART3_TX PB10
#define USART3_RX PB11
#define SPI2_SS PB12
#define SPI2_SCK PB13
#define SPI2_MISO PB14
#define SPI2_MOSI PB15

// ******** GPIOC ********
#define OTG_HS_ULPI_STP PC0
#define UNUSED_PC1 PC1
#define OTG_HS_ULPI_DIR PC2
#define OTG_HS_ULPI_NXT PC3
#define UNUSED_PC4 PC4
#define UNUSED_PC5 PC5
#define SDIO_D6 PC6
#define SDIO_D7 PC7
#define SDIO_D0 PC8
#define SDIO_D1 PC9
#define SDIO_D2 PC10
#define SDIO_D3 PC11
#define SDIO_CK PC12
#define SDIO_CMD PD2

#define SPI3_ALT_SCK PC10 // Conflicts with SDIO above
#define SPI3_ALT_MISO PC11
#define SPI3_ALT_MOSI PC12

#define _32k_OSC_IN _PC14 // 32kHz LSE oscillator. DO NOT USE!
#define _32k_OSC_OUT _PC15

// Constants for RTC sycn/async prescalars. Works for LSE at 32768Hz
#define RTC_ASYNCH_PREDIV       0x7fU // 128
#define RTC_SYNCH_PREDIV        0x00ffU // * 256 = 32768

#endif /* LIB_MATCHBOX_MB415_H_ */
