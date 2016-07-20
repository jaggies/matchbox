/*
 * spi.h
 *
 *  Created on: Jul 12, 2016
 *      Author: jmiller
 */

#ifndef SPI_H_
#define SPI_H_

#include "stm32f4xx_hal_dma.h"
#include "stm32f4xx_hal_spi.h"

extern "C" void SPI1_IRQHandler();
extern "C" void SPI2_IRQHandler();
extern "C" void SPI3_IRQHandler();

class Spi {
    public:
        static const int DEFAULT_TIMEOUT = 1000; // timeout for sync transfer, in milliseconds
        enum Status { OK = 0U, ERROR, BUSY, TIMEOUT, ILLEGAL};

        enum Bus { SP1 = 0, SP2 = 1, SP3 = 2 };
        enum Mode { Master = 0, Slave = 1 };
        enum Direction { TxRx = 0, RxOnly, BiDi };
        enum Size { S8, S16 };
        enum Polarity { LOW, HIGH };
        enum Phase { Rising, Falling };
        enum Order { MSB_FIRST, LSB_FIRST };

        class Config {
            public:
                Config() : mode(Master), dir(TxRx), size(S8), polarity(LOW),
                        phase(Rising), order(MSB_FIRST) { }
                Config& setMode(Mode m) { mode = m; return *this; }
                Config& setDirection(Direction d) { dir = d; return *this; }
                Config& setSize(Size sz) { size = sz; return *this; }
                Config& setPolarity(Polarity p) { polarity = p; return *this; }
                Config& setPhase(Phase p) { phase = p; return *this; }
                Config& setOrder(Order ord) { order = ord; return *this; }

                Mode mode;
                Direction dir;
                Size size;
                Polarity polarity;
                Phase phase;
                Order order;
        };

        typedef void (*TransmitCallback)(void *args);
        typedef void (*ReceiveCallback)(void *args);
        typedef void (*TxRxCallback)(void *args);

        Spi(Bus bus, const Config& config);
        ~Spi();

        Status transmit(const uint8_t* data, uint16_t n, TransmitCallback cb = 0, void* args = 0);
        Status receive(uint8_t* data, uint16_t n, ReceiveCallback cb = 0, void* args = 0);
        Status txrx(const uint8_t* tx, uint8_t* rx, uint16_t n, TxRxCallback cb = 0, void* args = 0);

    private:
        friend void SPI1_IRQHandler();
        friend void SPI2_IRQHandler();
        friend void SPI3_IRQHandler();
        friend void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi);
        friend void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi);
        friend void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi);
        void transmitComplete();
        void receiveComplete();
        void txrxComplete();
        void handleIrq();
    private:
        SPI_HandleTypeDef _spi;
        Bus _bus;
        TransmitCallback _txCallback;
        ReceiveCallback _rxCallback;
        TxRxCallback _txrxCallback;
        void* _txArgs;
        void* _rxArgs;
        void* _txrxArgs;
};

#endif /* SPI_H_ */
