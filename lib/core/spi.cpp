/*
 * spi.cpp
 *
 *  Created on: Jul 12, 2016
 *      Author: jmiller
 */
#include <string.h> // memset
#include "stm32.h"
#include "spi.h"
#include "util.h"


static SPI_TypeDef* const _busMap[] = { SPI1, SPI2, SPI3 };
static const IRQn_Type irqMap[] = { SPI1_IRQn, SPI2_IRQn, SPI3_IRQn };
static const uint32_t _dirMap[] = { SPI_DIRECTION_2LINES, SPI_DIRECTION_2LINES_RXONLY,
        SPI_DIRECTION_1LINE };
static const uint32_t clkdivs[] = {
    SPI_BAUDRATEPRESCALER_2, SPI_BAUDRATEPRESCALER_4, SPI_BAUDRATEPRESCALER_8,
    SPI_BAUDRATEPRESCALER_16, SPI_BAUDRATEPRESCALER_32, SPI_BAUDRATEPRESCALER_64,
    SPI_BAUDRATEPRESCALER_128, SPI_BAUDRATEPRESCALER_256
};

// Used to quickly map an irq to an instance of Spi object
static Spi* spiMap[3] = { 0, 0, 0 };

Spi::Spi(Bus bus, const Config& config)
        : _bus(bus), _txCallback(0), _rxCallback(0), _txrxCallback(0), _txArgs(0),
          _rxArgs(0), _txrxArgs(0) {
    bzero(&_spi, sizeof(_spi));
    _spi.Instance = _busMap[bus];
    _spi.Init.Mode = config.mode == Master ? SPI_MODE_MASTER : SPI_MODE_SLAVE;
    _spi.Init.Direction = _dirMap[config.dir];
    _spi.Init.DataSize = config.size == S8 ? SPI_DATASIZE_8BIT : SPI_DATASIZE_16BIT;
    _spi.Init.CLKPolarity = config.polarity == LOW ? SPI_POLARITY_LOW : SPI_POLARITY_HIGH;
    _spi.Init.CLKPhase = config.phase == Rising ? SPI_PHASE_1EDGE : SPI_PHASE_2EDGE;
    _spi.Init.NSS = config.ss == SOFTWARE ? SPI_NSS_SOFT
            : (config.ss == HARD_OUTPUT ? SPI_NSS_HARD_OUTPUT : SPI_NSS_HARD_INPUT);
    _spi.Init.BaudRatePrescaler = clkdivs[config.div];
    _spi.Init.FirstBit = config.order == MSB_FIRST ? SPI_FIRSTBIT_MSB : SPI_FIRSTBIT_LSB;
    _spi.Init.TIMode = SPI_TIMODE_DISABLE;
    _spi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    _spi.Init.CRCPolynomial = 10;
    HAL_SPI_Init(&_spi);

    // Enable interrupts to allow async transmit/receive
    HAL_NVIC_SetPriority(irqMap[bus], 15 /* low preempt priority */, 0 /* high sub-priority*/);
    HAL_NVIC_EnableIRQ(irqMap[bus]);

    // Keep track of Spi instance
    spiMap[bus] = this;
}

Spi::~Spi() {
    HAL_SPI_DeInit(&_spi);
    HAL_NVIC_DisableIRQ(irqMap[_bus]);
    spiMap[_bus] = NULL; // mark unused
}

inline void Spi::transmitComplete() {
    if (_txCallback) {
        (*_txCallback)(_txArgs);
    }
}

inline void Spi::receiveComplete() {
    if (_rxCallback) {
        (*_rxCallback)(_rxArgs);
    }
}

inline void Spi::txrxComplete() {
    if (_txrxCallback) {
        (*_txrxCallback)(_txrxArgs);
    }
}

void Spi::handleIrq() {
    HAL_SPI_IRQHandler(&_spi);
}

Spi::Status Spi::transmit(const uint8_t* data, uint16_t n, TransmitCallback cb, void* args) {
    HAL_StatusTypeDef status;
    if (!data) return ILLEGAL;
    if (n < 1) return OK; // nothing to do
    if (cb) { // Use async spi transfer
        _txCallback = cb;
        _txArgs = args;
        return (Spi::Status) HAL_SPI_Transmit_IT(&_spi, const_cast<uint8_t*>(data), n);
    } else { // Use blocking spi transfer
        return (Spi::Status) HAL_SPI_Transmit(&_spi, const_cast<uint8_t*>(data), n, DEFAULT_TIMEOUT);
    }
}

Spi::Status Spi::receive(uint8_t* data, uint16_t n, ReceiveCallback cb, void* args) {
    if (!data) return ILLEGAL;
    if (n < 1) return OK; // nothing to do
    if (cb) { // Use async spi transfer
        _rxCallback = cb;
        _rxArgs = args;
        return (Spi::Status) HAL_SPI_Receive_IT(&_spi, data, n);
    } else { // Use blocking spi transfer
        return (Spi::Status) HAL_SPI_Receive(&_spi, data, n, DEFAULT_TIMEOUT);
    }

}

Spi::Status Spi::txrx(const uint8_t* txData, uint8_t* rxData, uint16_t n, TxRxCallback cb, void* args) {
    if (!txData || !rxData) return ILLEGAL;
    if (n < 1) return OK; // nothing to do
    if (cb) { // Use async spi transfer
        _txrxCallback = cb;
        _txrxArgs = args;
        return (Spi::Status) HAL_SPI_TransmitReceive_IT(&_spi, const_cast<uint8_t*>(txData), rxData, n);
    } else { // Use blocking spi transfer
        return (Spi::Status) HAL_SPI_TransmitReceive(&_spi, const_cast<uint8_t*>(txData), rxData, n, DEFAULT_TIMEOUT);
    }
}

extern "C" void SPI1_IRQHandler()
{
    HAL_NVIC_ClearPendingIRQ(SPI1_IRQn);
    if (spiMap[0]) {
        spiMap[0]->handleIrq();
    }
}

extern "C" void SPI2_IRQHandler()
{
    HAL_NVIC_ClearPendingIRQ(SPI2_IRQn);
    if (spiMap[1]) {
        spiMap[1]->handleIrq();
    }
}

extern "C" void SPI3_IRQHandler()
{
    HAL_NVIC_ClearPendingIRQ(SPI3_IRQn);
    if (spiMap[2]) {
        spiMap[2]->handleIrq();
    }
}

extern "C" void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
    for (int i = 0; i < Number(spiMap); i++) {
        if (spiMap[i] && &spiMap[i]->_spi == hspi) {
            spiMap[i]->transmitComplete();
            break;
        }
    }
}

extern "C" void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)
{
    for (int i = 0; i < Number(spiMap); i++) {
        if (spiMap[i] && &spiMap[i]->_spi == hspi) {
            spiMap[i]->receiveComplete();
            break;
        }
    }
}

extern "C" void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    for (int i = 0; i < Number(spiMap); i++) {
        if (spiMap[i] && &spiMap[i]->_spi == hspi) {
            spiMap[i]->txrxComplete();
            break;
        }
    }
}


