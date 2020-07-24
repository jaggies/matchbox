////////////////////////////////////////////////////////////////////////////////
//
// Auto-generated file, DO NOT EDIT!!!
//
// Command line:
//  ./bin/makepinheader.py -p target/stm32f4xx/stm32f415/stm32f415.csv -h lib/core/pinmap.h
//
////////////////////////////////////////////////////////////////////////////////
#ifndef  PINMAP_H
#define  PINMAP_H


enum Pins {
	PA0 = 0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PA8, PA9, PA10, PA11, PA12, PA13, PA14, PA15,
	PB0, PB1, PB2, PB3, PB4, PB5, PB6, PB7, PB8, PB9, PB10, PB11, PB12, PB13, PB14, PB15,
	PC0, PC1, PC2, PC3, PC4, PC5, PC6, PC7, PC8, PC9, PC10, PC11, PC12, PC13, _PC14, _PC15,
	PD0, PD1, PD2, PD3, PD4, PD5, PD6, PD7, PD8, PD9, PD10, PD11, PD12, PD13, PD14, PD15,
	PE0, PE1, PE2, PE3, PE4, PE5, PE6, PE7, PE8, PE9, PE10, PE11, PE12, PE13, PE14, PE15,
	PF0, PF1, PF2, PF3, PF4, PF5, PF6, PF7, PF8, PF9, PF10, PF11, PF12, PF13, PF14, PF15,
	PG0, PG1, PG2, PG3, PG4, PG5, PG6, PG7, PG8, PG9, PG10, PG11, PG12, PG13, PG14, PG15,
	PH0, PH1, PH2, PH3, PH4, PH5, PH6, PH7, PH8, PH9, PH10, PH11, PH12, PH13, PH14, PH15,
	PI0, PI1, PI2, PI3, PI4, PI5, PI6, PI7, PI8, PI9, PI10, PI11, PI12, PI13, PI14, PI15,
};


#define PIN(a) (1 << ((a) % 16))
extern GPIO_TypeDef* _gpioBanks[];
#define BANK(a) (_gpioBanks[(a) / 16])

#define	 SW1 	 PC13 	// Pin  2
#define	 SPI2_MISO 	 PC2 	// Pin  10
#define	 SPI2_MOSI 	 PC3 	// Pin  11
#define	 SPI1_NSS 	 PA4 	// Pin  20
#define	 SPI1_SCK 	 PA5 	// Pin  21
#define	 SPI1_MISO 	 PA6 	// Pin  22
#define	 SPI1_MOSI 	 PA7 	// Pin  23
#define	 VCC_MON 	 PC5 	// Pin  25
#define	 LED1 	 PB2 	// Pin  28
#define	 SPI2_SCK 	 PB10 	// Pin  29
#define	 SPI2_NSS 	 PB12 	// Pin  33
#define	 SDIO_D0 	 PC8 	// Pin  39
#define	 SDIO_D1 	 PC9 	// Pin  40
#define	 NRF8K_RDYN 	 PA9 	// Pin  42
#define	 USB_OTG_FS_ID 	 PA10 	// Pin  43
#define	 USB_OTG_FS_DM 	 PA11 	// Pin  44
#define	 USB_OTG_FS_DP 	 PA12 	// Pin  45
#define	 SW2 	 PA13 	// Pin  46
#define	 PWR_KILL 	 PA14 	// Pin  49
#define	 NRF8K_REQN 	 PA15 	// Pin  50
#define	 SDIO_D2 	 PC10 	// Pin  51
#define	 SDIO_D3 	 PC11 	// Pin  52
#define	 SDIO_CK 	 PC12 	// Pin  53
#define	 SDIO_CMD 	 PD2 	// Pin  54
#define	 NRF8K_SCK 	 PB3 	// Pin  55
#define	 NRF8K_MISO 	 PB4 	// Pin  56
#define	 NRF8K_MOSI 	 PB5 	// Pin  57
#define	 USART1_TX 	 PB6 	// Pin  58
#define	 USART1_RX 	 PB7 	// Pin  59
#define	 SD_DET 	 PB8 	// Pin  61

#endif //  PINMAP_H
