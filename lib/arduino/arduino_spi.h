/*
 * arduino_spi.h
 *
 *  Created on: Jul 20, 2016
 *      Author: jmiller
 */

#ifndef ARDUINO_SPI_H_
#define ARDUINO_SPI_H_

#define LSBFIRST 0
#define MSBFIRST 1

#define SPI_CLOCK_DIV4 0x00
#define SPI_CLOCK_DIV16 0x01
#define SPI_CLOCK_DIV64 0x02
#define SPI_CLOCK_DIV128 0x03
#define SPI_CLOCK_DIV2 0x04
#define SPI_CLOCK_DIV8 0x05
#define SPI_CLOCK_DIV32 0x06

#define SPI_MODE0 0x00
#define SPI_MODE1 0x04
#define SPI_MODE2 0x08
#define SPI_MODE3 0x0C

class SPISettings {
    public:
        SPISettings(uint32_t clk, uint8_t bitOrder, uint8_t dataMode) :
                clock(clk), bitOrder(bitOrder), dataMode(dataMode) {
        }
        SPISettings() :
                clock(4000000), bitOrder(MSBFIRST), dataMode(SPI_MODE0) {
        }
        friend class SPIClass;
    private:
        uint32_t clock;
        uint8_t bitOrder;
        uint8_t dataMode;
};

class Spi;

class SPIClass {
    public:
        SPIClass();
        static void begin();
        static void usingInterrupt(uint8_t interruptNumber);
        static void notUsingInterrupt(uint8_t interruptNumber);
        static void beginTransaction(SPISettings settings);
        static uint8_t transfer(uint8_t data);
        static uint16_t transfer16(uint16_t data);
        static void transfer(void *buf, size_t count);
        static void endTransaction(void);
        static void end();
        static void setBitOrder(uint8_t bitOrder); // Deprecated.  Use beginTransaction()
        static void setDataMode(uint8_t dataMode); // Deprecated.  Use beginTransaction()
        static void setClockDivider(uint8_t clockDiv); // Deprecated.  Use beginTransaction()
};

extern SPIClass SPI;

#endif /* ARDUINO_SPI_H_ */
