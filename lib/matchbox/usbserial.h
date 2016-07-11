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
#define USB_RX_DATA_SIZE  64
#define USB_TX_DATA_SIZE  64

class UsbSerial {
    public:
        static UsbSerial* getInstance() {
            return _instance ? _instance : (_instance = new UsbSerial());
        }
        int8_t transmit(uint8_t* buf, uint16_t len);
    private:
        UsbSerial();
        ~UsbSerial();

        // HAL interface methods
        int8_t init(void);
        int8_t deInit(void);
        int8_t control(uint8_t cmd, uint8_t* pbuf, uint16_t length);
        int8_t receive(uint8_t* buf, uint32_t *len);

    public: // TODO: Find a way to keep these private
        // HAL interface callback functions
        static int8_t Init_FS(void);
        static int8_t DeInit_FS(void);
        static int8_t Control_FS(uint8_t cmd, uint8_t* pbuf, uint16_t length);
        static int8_t Receive_FS(uint8_t* pbuf, uint32_t *len);

        uint8_t _rxBuffer[USB_RX_DATA_SIZE];
        uint8_t _txBuffer[USB_TX_DATA_SIZE];
        USBD_HandleTypeDef _usbDevice;
        static UsbSerial* _instance;
};
#else // not C++
extern int8_t usb_transmit(uint8_t* buf, uint16_t len);
#endif

#endif /* USBSERIAL_H_ */
