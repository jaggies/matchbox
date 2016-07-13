/*
 * spi.h
 *
 *  Created on: Jul 12, 2016
 *      Author: jmiller
 */

#ifndef SPI_H_
#define SPI_H_

extern "C" void SPI1_IRQHandler();
extern "C" void SPI2_IRQHandler();
extern "C" void SPI3_IRQHandler();

class Spi {
    public:
        static const int DEFAULT_TIMEOUT = 1000; // ms
        enum Bus { SP1 = 0, SP2 = 1, SP3 = 2 };
        enum Mode { Master = 0, Slave = 1 };
        enum Direction { TxRx = 0, RxOnly, BiDi };
        enum DataSize { S8, S16 };
        enum Polarity { LOW, HIGH };
        enum Phase { Rising, Falling };
        enum FirstBit { MSB, LSB };
        enum Status { OK = 0U, ERROR, BUSY, TIMEOUT, ILLEGAL};
        typedef void (*TransmitCallback)(void *args);
        typedef void (*ReceiveCallback)(void *args);
        typedef void (*TxRxCallback)(void *args);

        Spi(Bus bus, Mode mode = Master, Direction dir = TxRx, DataSize = S8, Polarity pol = LOW,
                Phase phase = Rising, FirstBit bit = MSB);
        ~Spi();

        Status transmit(uint8_t* data, uint16_t n, TransmitCallback cb = 0, void* args = 0);
        Status receive(uint8_t* data, uint16_t n, ReceiveCallback cb = 0, void* args = 0);
        Status txrx(uint8_t* tx, uint8_t* rx, uint16_t n, TxRxCallback cb = 0, void* args = 0);

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
        TransmitCallback _txCallback;
        ReceiveCallback _rxCallback;
        TxRxCallback _txrxCallback;
        void* _txArgs;
        void* _rxArgs;
        void* _txrxArgs;
};

#endif /* SPI_H_ */
