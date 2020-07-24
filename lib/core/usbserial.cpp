/*
 * usbserial.cpp
 *
 *  Created on: Jul 10, 2016
 *      Author: jmiller
 */
#include "cmsis_os.h"
#include "usbserial.h"
#include "usbd_desc.h" // matchbox custom usb device descriptor

UsbSerial* UsbSerial::_instance;

extern "C" {
    extern void write(int, uint8_t*, uint32_t); // TODO: Remove this stub!
}

USBD_CDC_ItfTypeDef UsbSerial::USBD_Interface_fops_FS = {
    UsbSerial::Init_FS,
    UsbSerial::DeInit_FS,
    UsbSerial::Control_FS,
    UsbSerial::Receive_FS,
	#ifdef STM32F4XX
    UsbSerial::TxComplete_FS 
	#endif
};

USBD_CDC_LineCodingTypeDef UsbSerial::linecoding = {
    115200, /* baud rate*/
    0x00, /* stop bits-1*/
    0x00, /* parity - none*/
    0x08 /* nb. of bits 8*/
};

int8_t usb_transmit(char* buf, size_t len) {
    return UsbSerial::getInstance()->transmit(buf, len);
}

size_t usb_receive(char* buf, size_t len) {
    return UsbSerial::getInstance()->receive(buf, len);
}

UsbSerial::UsbSerial() {
    memset(&_usbDevice, 0, sizeof(_usbDevice));
    USBD_Init(&_usbDevice, &Matchbox_USB_Desc, DEVICE_FS);
    USBD_RegisterClass(&_usbDevice, &USBD_CDC);
    USBD_CDC_RegisterInterface(&_usbDevice, &USBD_Interface_fops_FS);
    USBD_Start(&_usbDevice);
}

UsbSerial::~UsbSerial() {
}

int8_t UsbSerial::Init_FS(void) {
    /* Set Application Buffers */
    UsbSerial* thisPtr = UsbSerial::getInstance();
    USBD_CDC_SetTxBuffer(&thisPtr->_usbDevice, thisPtr->_txBuffer, sizeof(thisPtr->_txBuffer));
    USBD_CDC_SetRxBuffer(&thisPtr->_usbDevice, thisPtr->_rxBuffer);
    return (USBD_OK);
}

int8_t UsbSerial::DeInit_FS(void) {
    return (USBD_OK); // TODO
}

/**
 * @brief  CDC_Control_FS
 *         Manage the CDC class requests
 * @param  cmd: Command code
 * @param  pbuf: Buffer containing command data (request parameters)
 * @param  length: Number of data to be sent (in bytes)
 * @retval Result of the operation: USBD_OK if all operations are OK else USBD_FAIL
 */
/*******************************************************************************/
 /* Line Coding Structure                                                       */
 /*-----------------------------------------------------------------------------*/
 /* Offset | Field       | Size | Value  | Description                          */
 /* 0      | dwDTERate   |   4  | Number |Data terminal rate, in bits per second*/
 /* 4      | bCharFormat |   1  | Number | Stop bits                            */
 /*                                        0 - 1 Stop bit                       */
 /*                                        1 - 1.5 Stop bits                    */
 /*                                        2 - 2 Stop bits                      */
 /* 5      | bParityType |  1   | Number | Parity                               */
 /*                                        0 - None                             */
 /*                                        1 - Odd                              */
 /*                                        2 - Even                             */
 /*                                        3 - Mark                             */
 /*                                        4 - Space                            */
 /* 6      | bDataBits  |   1   | Number Data bits (5, 6, 7, 8 or 16).          */
 /*******************************************************************************/
int8_t UsbSerial::Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length) {
    switch (cmd) {
        case CDC_SEND_ENCAPSULATED_COMMAND:
        break;
        case CDC_GET_ENCAPSULATED_RESPONSE:
        break;
        case CDC_SET_COMM_FEATURE:
        break;
        case CDC_GET_COMM_FEATURE:
        break;
        case CDC_CLEAR_COMM_FEATURE:
        break;
        case CDC_SET_LINE_CODING: // see above
            linecoding.bitrate = (uint32_t) (pbuf[0] | (pbuf[1] << 8) | (pbuf[2] << 16)
                    | (pbuf[3] << 24));
            linecoding.format = pbuf[4];
            linecoding.paritytype = pbuf[5];
            linecoding.datatype = pbuf[6];
        break;
        case CDC_GET_LINE_CODING: // see above
            linecoding.bitrate = (uint32_t) (pbuf[0] | (pbuf[1] << 8) | (pbuf[2] << 16)
                    | (pbuf[3] << 24));
            linecoding.format = pbuf[4];
            linecoding.paritytype = pbuf[5];
            linecoding.datatype = pbuf[6];
        break;
        case CDC_SET_CONTROL_LINE_STATE:
        break;
        case CDC_SEND_BREAK:
        break;
        default:
        break;
    }
    return USBD_OK;
}

int8_t UsbSerial::Receive_FS(uint8_t *buf, uint32_t *len) {
    UsbSerial* thisPtr = UsbSerial::getInstance();
    if (thisPtr->_usbDevice.dev_state != USBD_STATE_CONFIGURED) {
        return USBD_FAIL;
    }

    if (!buf || !len || *len <= 0) {
        return USBD_FAIL;
    }

    uint8_t result = USBD_OK;
    do {
        result = USBD_CDC_SetRxBuffer(&thisPtr->_usbDevice, &buf[0]);
    } while (result != USBD_OK);

    do {
        result = USBD_CDC_ReceivePacket(&thisPtr->_usbDevice);
    } while (result != USBD_OK);

    while ((*len)--) {
        if (thisPtr->_readFifo.isFull()) {
            return USBD_FAIL;  // overrun
        } else {
            thisPtr->_readFifo.add(*buf++);
        }
    }
    return (USBD_OK);
}

int8_t UsbSerial::TxComplete_FS(uint8_t *Buf, uint32_t *Len, uint8_t epnum) {
    return USBD_OK;
}

int8_t UsbSerial::transmit(char* buf, size_t len) {
    USBD_CDC_HandleTypeDef *hcdc = (USBD_CDC_HandleTypeDef*) _usbDevice.pClassData;
    if (hcdc->TxState != 0) {
        return USBD_BUSY;
    }
    USBD_CDC_SetTxBuffer(&_usbDevice, (uint8_t*) buf, len);
    return USBD_CDC_TransmitPacket(&_usbDevice);
}

size_t UsbSerial::receive(char* buf, size_t nchar) {
    size_t nread = 0;
    while (nchar) {
        if (!_readFifo.isEmpty()) {
            nread++;
            _readFifo.remove((char*) buf++);
            nchar--;
        } else {
            osThreadYield();
        }
    }
    return nread;
}
