/*
 * usbserial.h
 *
 *  Created on: Jul 10, 2016
 *      Author: jmiller
 */

#ifndef USBSERIAL_H_
#define USBSERIAL_H_

#include "usbd_def.h"
#include "usbd_cdc.h"

#ifdef __cplusplus

#include "fifo.h"

#define USB_RX_DATA_SIZE  64
#define USB_TX_DATA_SIZE  64

class UsbSerial {
    public:
        static UsbSerial* getInstance() {
            return _instance ? _instance : (_instance = new UsbSerial());
        }
        // Transmits len bytes. Returns USBD_BUSY if there's already a transmission in progress.
        int8_t transmit(char* buf, size_t len);

        // Reads received data from Fifo (filled by Receive_FS)
        size_t receive(char* buf, size_t nchar);
    private:
        UsbSerial();
        ~UsbSerial();

        // USB FullSpeed operations
        static USBD_CDC_ItfTypeDef USBD_Interface_fops_FS;
        static USBD_CDC_LineCodingTypeDef linecoding;
        static int8_t Init_FS(void);
        static int8_t DeInit_FS(void);
        static int8_t Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
        static int8_t Receive_FS(uint8_t* pbuf, uint32_t *len);
        static int8_t TxComplete_FS(uint8_t *Buf, uint32_t *Len, uint8_t epnum);

        uint8_t _rxBuffer[USB_RX_DATA_SIZE];
        uint8_t _txBuffer[USB_TX_DATA_SIZE];
        USBD_HandleTypeDef _usbDevice;
        Fifo<char, size_t, USB_RX_DATA_SIZE> _readFifo;
        static UsbSerial* _instance;
};

extern "C" {
    // Ensure these have C bindings
    int8_t usb_transmit(char* buf, size_t len);
    size_t usb_receive(char* buf, size_t len);
};

#else // !__cplusplus
    // Declare these for C apps
    int8_t usb_transmit(char* buf, size_t len);
    size_t usb_receive(char* buf, size_t len);
#endif

#endif /* USBSERIAL_H_ */
