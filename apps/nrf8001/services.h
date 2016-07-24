/**
* This file is autogenerated by nRFgo Studio 1.21.1.3
*/

#ifndef SETUP_MESSAGES_H__
#define SETUP_MESSAGES_H__

#include "hal_platform.h"
#include "aci.h"


#define SETUP_ID 0
#define SETUP_FORMAT 3 /** nRF8001 D */
#define ACI_DYNAMIC_DATA_SIZE 222

/* Service: Gap - Characteristic: Device name - Pipe: SET */
#define PIPE_GAP_DEVICE_NAME_SET          1
#define PIPE_GAP_DEVICE_NAME_SET_MAX_SIZE 8

/* Service: GATT - Characteristic: Service Changed - Pipe: TX_ACK */
#define PIPE_GATT_SERVICE_CHANGED_TX_ACK          2
#define PIPE_GATT_SERVICE_CHANGED_TX_ACK_MAX_SIZE 4

/* Service: Device Information - Characteristic: Hardware Revision String - Pipe: SET */
#define PIPE_DEVICE_INFORMATION_HARDWARE_REVISION_STRING_SET          3
#define PIPE_DEVICE_INFORMATION_HARDWARE_REVISION_STRING_SET_MAX_SIZE 2

/* Service: SensorService - Characteristic: Status - Pipe: TX */
#define PIPE_SENSORSERVICE_STATUS_TX          4
#define PIPE_SENSORSERVICE_STATUS_TX_MAX_SIZE 20

/* Service: SensorService - Characteristic: Channel0 - Pipe: TX */
#define PIPE_SENSORSERVICE_CHANNEL0_TX          5
#define PIPE_SENSORSERVICE_CHANNEL0_TX_MAX_SIZE 20

/* Service: SensorService - Characteristic: Channel1 - Pipe: TX */
#define PIPE_SENSORSERVICE_CHANNEL1_TX          6
#define PIPE_SENSORSERVICE_CHANNEL1_TX_MAX_SIZE 20


#define NUMBER_OF_PIPES 6

#define SERVICES_PIPE_TYPE_MAPPING_CONTENT {\
  {ACI_STORE_LOCAL, ACI_SET},   \
  {ACI_STORE_LOCAL, ACI_TX_ACK},   \
  {ACI_STORE_LOCAL, ACI_SET},   \
  {ACI_STORE_LOCAL, ACI_TX},   \
  {ACI_STORE_LOCAL, ACI_TX},   \
  {ACI_STORE_LOCAL, ACI_TX},   \
}

#define GAP_PPCP_MAX_CONN_INT 0xffff /**< Maximum connection interval as a multiple of 1.25 msec , 0xFFFF means no specific value requested */
#define GAP_PPCP_MIN_CONN_INT  0xffff /**< Minimum connection interval as a multiple of 1.25 msec , 0xFFFF means no specific value requested */
#define GAP_PPCP_SLAVE_LATENCY 0
#define GAP_PPCP_CONN_TIMEOUT 0xffff /** Connection Supervision timeout multiplier as a multiple of 10msec, 0xFFFF means no specific value requested */

#define NB_SETUP_MESSAGES 32
#define SETUP_MESSAGES_CONTENT {\
    {0x00,\
        {\
            0x07,0x06,0x00,0x00,0x03,0x02,0x42,0x07,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x00,0x06,0x01,0x01,0x00,0x01,0x06,0x00,0x08,\
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x10,0x1c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10,0x03,0x90,0x01,0xff,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x10,0x38,0xff,0xff,0x02,0x58,0x0a,0x05,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x05,0x06,0x10,0x54,0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x00,0x04,0x04,0x02,0x02,0x00,0x01,0x28,0x00,0x01,0x00,0x18,0x04,0x04,0x05,0x05,0x00,\
            0x02,0x28,0x03,0x01,0x0e,0x03,0x00,0x00,0x2a,0x04,0x14,0x08,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x1c,0x08,0x00,0x03,0x2a,0x00,0x01,0x4d,0x61,0x74,0x63,0x68,0x42,0x6f,0x78,0x04,0x04,\
            0x05,0x05,0x00,0x04,0x28,0x03,0x01,0x02,0x05,0x00,0x01,0x2a,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x38,0x06,0x04,0x03,0x02,0x00,0x05,0x2a,0x01,0x01,0x00,0x00,0x04,0x04,0x05,0x05,0x00,\
            0x06,0x28,0x03,0x01,0x02,0x07,0x00,0x04,0x2a,0x06,0x04,0x09,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x54,0x08,0x00,0x07,0x2a,0x04,0x01,0xff,0xff,0xff,0xff,0x00,0x00,0xff,0xff,0x04,0x04,\
            0x02,0x02,0x00,0x08,0x28,0x00,0x01,0x01,0x18,0x04,0x04,0x05,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x70,0x05,0x00,0x09,0x28,0x03,0x01,0x22,0x0a,0x00,0x05,0x2a,0x26,0x04,0x05,0x04,0x00,\
            0x0a,0x2a,0x05,0x01,0x00,0x00,0x00,0x00,0x46,0x14,0x03,0x02,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0x8c,0x00,0x0b,0x29,0x02,0x01,0x00,0x00,0x04,0x04,0x02,0x02,0x00,0x0c,0x28,0x00,0x01,\
            0x0a,0x18,0x04,0x04,0x05,0x05,0x00,0x0d,0x28,0x03,0x01,0x02,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0xa8,0x0e,0x00,0x24,0x2a,0x06,0x04,0x09,0x08,0x00,0x0e,0x2a,0x24,0x01,0x4d,0x61,0x74,\
            0x63,0x68,0x42,0x6f,0x78,0x06,0x04,0x08,0x07,0x00,0x0f,0x29,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0xc4,0x04,0x01,0x19,0x00,0x00,0x00,0x01,0x00,0x00,0x04,0x04,0x05,0x05,0x00,0x10,0x28,\
            0x03,0x01,0x02,0x11,0x00,0x27,0x2a,0x06,0x04,0x03,0x02,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0xe0,0x11,0x2a,0x27,0x01,0x30,0x31,0x06,0x04,0x08,0x07,0x00,0x12,0x29,0x04,0x01,0x19,\
            0x00,0x00,0x00,0x01,0x00,0x00,0x04,0x04,0x05,0x05,0x00,0x13,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x20,0xfc,0x28,0x03,0x01,0x02,0x14,0x00,0x29,0x2a,0x06,0x04,0x15,0x14,0x00,0x14,0x2a,0x29,\
            0x01,0x48,0x61,0x63,0x6b,0x44,0x6f,0x67,0x2e,0x6e,0x65,0x74,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x21,0x18,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x06,0x04,0x08,0x07,0x00,0x15,0x29,\
            0x04,0x01,0x19,0x00,0x00,0x00,0x01,0x00,0x00,0x04,0x04,0x05,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x21,0x34,0x05,0x00,0x16,0x28,0x03,0x01,0x02,0x17,0x00,0x27,0x2a,0x06,0x04,0x05,0x04,0x00,\
            0x17,0x2a,0x27,0x01,0x30,0x2e,0x31,0x30,0x06,0x04,0x08,0x07,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x21,0x50,0x00,0x18,0x29,0x04,0x01,0x19,0x00,0x00,0x00,0x01,0x00,0x00,0x04,0x04,0x10,0x10,\
            0x00,0x19,0x28,0x00,0x01,0x0c,0xa7,0x3c,0xca,0x95,0xe7,0xd8,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x21,0x6c,0xbb,0x02,0x4f,0xbe,0xb2,0x00,0x00,0xf9,0x6f,0x04,0x04,0x13,0x13,0x00,0x1a,0x28,\
            0x03,0x01,0x10,0x1b,0x00,0x0c,0xa7,0x3c,0xca,0x95,0xe7,0xd8,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x21,0x88,0xbb,0x02,0x4f,0xbe,0xb2,0x01,0x00,0xf9,0x6f,0x16,0x00,0x15,0x14,0x00,0x1b,0x00,\
            0x01,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x21,0xa4,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x46,0x14,0x03,0x02,0x00,0x1c,\
            0x29,0x02,0x01,0x00,0x00,0x04,0x04,0x13,0x13,0x00,0x1d,0x28,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x21,0xc0,0x03,0x01,0x10,0x1e,0x00,0x0c,0xa7,0x3c,0xca,0x95,0xe7,0xd8,0xbb,0x02,0x4f,0xbe,\
            0xb2,0x00,0x01,0xf9,0x6f,0x16,0x00,0x15,0x14,0x00,0x1e,0x01,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x21,0xdc,0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
            0x00,0x00,0x00,0x00,0x00,0x00,0x46,0x14,0x03,0x02,0x00,0x1f,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x21,0xf8,0x29,0x02,0x01,0x00,0x00,0x04,0x04,0x13,0x13,0x00,0x20,0x28,0x03,0x01,0x10,0x21,\
            0x00,0x0c,0xa7,0x3c,0xca,0x95,0xe7,0xd8,0xbb,0x02,0x4f,0xbe,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x22,0x14,0xb2,0x01,0x01,0xf9,0x6f,0x14,0x00,0x14,0x00,0x00,0x21,0x01,0x01,0x02,0x00,0x00,\
            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x15,0x06,0x22,0x30,0x00,0x00,0x00,0x00,0x00,0x00,0x46,0x14,0x03,0x02,0x00,0x22,0x29,0x02,0x01,0x00,\
            0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x40,0x00,0x2a,0x00,0x01,0x00,0x80,0x04,0x00,0x03,0x00,0x00,0x2a,0x05,0x01,0x00,0x04,0x04,\
            0x00,0x0a,0x00,0x0b,0x2a,0x27,0x01,0x00,0x80,0x04,0x00,0x11,\
        },\
    },\
    {0x00,\
        {\
            0x1f,0x06,0x40,0x1c,0x00,0x00,0x00,0x01,0x02,0x00,0x02,0x04,0x00,0x1b,0x00,0x1c,0x01,0x00,0x02,0x00,\
            0x02,0x04,0x00,0x1e,0x00,0x1f,0x01,0x01,0x02,0x00,0x02,0x04,\
        },\
    },\
    {0x00,\
        {\
            0x07,0x06,0x40,0x38,0x00,0x21,0x00,0x22,\
        },\
    },\
    {0x00,\
        {\
            0x13,0x06,0x50,0x00,0x0c,0xa7,0x3c,0xca,0x95,0xe7,0xd8,0xbb,0x02,0x4f,0xbe,0xb2,0x00,0x00,0xf9,0x6f,\
        },\
    },\
    {0x00,\
        {\
            0x15,0x06,0x60,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,\
            0x00,0x00,\
        },\
    },\
    {0x00,\
        {\
            0x06,0x06,0xf0,0x00,0x03,0xbd,0x83,\
        },\
    },\
}

#endif
