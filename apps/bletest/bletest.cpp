/*
 * bletest.cpp
 *
 *  Created on: Jul 19, 2016
 *      Author: jmiller
 */
#include <math.h>
#include "lib_aci.h"
#include "aci_setup.h"
#include "matchbox.h"
#include "pin.h"
#include "spi.h"
#include "adc.h"
#include "fifo.h"
#include "util.h"
#include "services.h" // configuration file for nrf8001

osThreadId defaultTaskHandle;
void StartDefaultTask(void const * argument);
void nrf8001Setup();
void aci_loop();

#ifdef DEBUG_BLE
static const bool debug_ble = true;
#else
static const bool debug_ble = false;
#endif

template<class K> bool sendData(int pipe, const K& data);

struct Status {
    Status() : packetHigh(0xffff), sampleRate(250), channels(8) { }
    uint16_t packetHigh; // upper 16 bits of current packet
    uint16_t sampleRate; // in Hz
    uint8_t  channels;
};

struct Channel {
    Channel() : packetLow(0) { bzero(data, sizeof(data)); }
    uint16_t packetLow; // lower 16 bits of current packet
    int8_t data[18];
};

float rate = 0.0f;
static void adcCallback(const uint16_t* values, int n, void* arg) {
    static uint8_t x = 0;
    Fifo<uint8_t, int8_t, 32>& fifo = *(Fifo<uint8_t, int8_t, 32>*)arg;
//    while (n-- && fifo.add(*values++))
//        ;
    if (fifo.add(x)) {
        x++;
    }
    static uint32_t last = 0;
    if (last == 0) {
        last = HAL_GetTick();
    } else {
        uint32_t current = HAL_GetTick();
        float delta = (current - last) / 1000.0f; // in ms
        last = current;
        const float alpha = 0.00001;
        rate = (rate == 0.0f) ? delta : (alpha*delta + (1.0f-alpha)*rate);
    }
}

int main(void) {
    MatchBox* mb = new MatchBox(MatchBox::C72MHz);

    osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 2048);
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */
    while (1)
        ;
    return 0;
}

#ifdef SERVICES_PIPE_TYPE_MAPPING_CONTENT
static services_pipe_type_mapping_t services_pipe_type_mapping[NUMBER_OF_PIPES] =
        SERVICES_PIPE_TYPE_MAPPING_CONTENT;
#else
#define NUMBER_OF_PIPES 0
static services_pipe_type_mapping_t * services_pipe_type_mapping = NULL;
#endif

const uint16_t MIN_INTERVAL = 5; // *1.5ms
const uint16_t MAX_INTERVAL = 1000; // *1.5ms
const uint16_t SLAVE_LATENCY = 0; // #intervals nRF8001 can't transmit (higher = more power saving)
const uint16_t SLAVE_TIMEOUT = 60000; // 600 * 10ms = 6000ms = 6s
static const hal_aci_data_t setup_msgs[NB_SETUP_MESSAGES] = SETUP_MESSAGES_CONTENT;
static struct aci_state_t aci_state;

void StartDefaultTask(void const * argument) {
    Pin led(LED1, Pin::Config().setMode(Pin::MODE_OUTPUT));
    nrf8001Setup();
    Status status;
    Channel chan;
    enum Pipe {CHAN0, CHAN1, CHAN2, CHAN3, STATUS, LAST };
    float alpha = 0.0f;
    float dAlpha = 2.0f * M_PI / 1023;
    int packetCount = 0;
    int lastStatus = -1;
    Pipe pipe = STATUS;
    while (1) {
        aci_loop();
        if (aci_state.bonded) {
            if ((packetCount >> 16) != lastStatus) {
                lastStatus = packetCount >> 16;
                status.channels = 4;
                status.packetHigh = lastStatus;
                status.sampleRate = 250;
                pipe = STATUS;
            } else {
                for (int i = 0; i < sizeof(chan.data); i++) {
                    chan.data[i] = 127*sin(alpha);
                    alpha += dAlpha;
                }
                if (sendData(PIPE_SENSORSERVICE_CHANNEL0_TX, chan)) {
                    led.write(packetCount++ & 1);
                } else {
                    debug("Failed to send chan0\n");
                }
            }
            osDelay(100);
        } // bonded
    }
}

// Temporary buffers for sending ACI commands
static hal_aci_evt_t aci_data;
//static hal_aci_data_t aci_cmd;

// Timing change state variable
static bool timing_change_done = false;

void __ble_assert(const char *file, uint16_t line) {
    if (debug_ble) printf("BLE ERROR: file %s, line %d\n", file, line);
    while (1)
        ;
}

void nrf8001Setup() {
    osDelay(100); // give device time to reset

    // Point ACI data structures to the the setup data that the nRFgo studio generated for the nRF8001
    aci_state.aci_setup_info.services_pipe_type_mapping =
            services_pipe_type_mapping ? &services_pipe_type_mapping[0] : NULL;
    aci_state.aci_setup_info.number_of_pipes = NUMBER_OF_PIPES;
    aci_state.aci_setup_info.setup_msgs = setup_msgs;
    aci_state.aci_setup_info.num_setup_msgs = NB_SETUP_MESSAGES;
    aci_state.aci_pins.board_name = BOARD_DEFAULT; //See boards.h for details
    aci_state.aci_pins.reqn_pin = NRF8K_REQN; // SS pin for nRF8001
    aci_state.aci_pins.rdyn_pin = NRF8K_RDYN; // Interrupt pin from nRF8001
    aci_state.aci_pins.mosi_pin = NRF8K_MOSI; // SPI data -> nRF8001
    aci_state.aci_pins.miso_pin = NRF8K_MISO; // SPI data <- nRF8001
    aci_state.aci_pins.sck_pin = NRF8K_SCK; // SPI clk
    aci_state.aci_pins.spi_clock_divider = 0; // TODO: Should be ~2MHz
    aci_state.aci_pins.reset_pin = UNUSED_PIN;
    aci_state.aci_pins.active_pin = UNUSED_PIN;
    aci_state.aci_pins.optional_chip_sel_pin = UNUSED_PIN;
    aci_state.aci_pins.interface_is_interrupt = false; // use interrupt handler for RDY#, else poll
    aci_state.aci_pins.interrupt_number = 1;

    // Reset nRF8001 with hardware line, if available.
    lib_aci_init(&aci_state, false /* debug_ble */);
}

void aci_loop() {
    static bool setup_required = false;

    // We enter the if statement only when there is a ACI event available to be processed
    if (lib_aci_event_get(&aci_state, &aci_data)) {
        aci_evt_t * aci_evt;
        aci_evt = &aci_data.evt;

        switch (aci_evt->evt_opcode) {
            // As soon as you reset the nRF8001 you will get an ACI Device Started Event
            case ACI_EVT_DEVICE_STARTED: {
                aci_state.data_credit_total = aci_evt->params.device_started.credit_available;
                if (debug_ble) printf("Total credits: %d\n", aci_evt->params.device_started.credit_available);
                switch (aci_evt->params.device_started.device_mode) {
                    case ACI_DEVICE_SETUP:
                        // When the device is in the setup mode
                        if (debug_ble) printf("Evt Device Started: Setup\n");
                        setup_required = true;
                    break;

                    case ACI_DEVICE_STANDBY:
                        if (debug_ble) printf("Evt Device Started: Standby\n");
                        //Looking for an iPhone by sending radio advertisements
                        //When an iPhone connects to us we will get an ACI_EVT_CONNECTED event from the nRF8001
                        if (aci_evt->params.device_started.hw_error) {
                            osDelay(20); //Magic number used to make sure the HW error event is handled correctly.
                        } else {
                            lib_aci_connect(180 /* seconds */, 0x0050 /* advertising interval 50ms*/);
                            if (debug_ble) printf("Advertising started\n");
                        }
                    break;
                }
            }
            break; //ACI Device Started Event

            case ACI_EVT_CMD_RSP:
                //If an ACI command response event comes with an error -> stop
                if (ACI_STATUS_SUCCESS != aci_evt->params.cmd_rsp.cmd_status) {
                    //ACI ReadDynamicData and ACI WriteDynamicData will have status codes of
                    //TRANSACTION_CONTINUE and TRANSACTION_COMPLETE
                    //all other ACI commands will have status code of ACI_STATUS_SCUCCESS for a successful command
                    if (debug_ble) printf("ACI Command %x Status %x\n", aci_evt->params.cmd_rsp.cmd_opcode,
                            aci_evt->params.cmd_rsp.cmd_status);
                }
                if (ACI_CMD_GET_DEVICE_VERSION == aci_evt->params.cmd_rsp.cmd_opcode) {
                    //Store the version and configuration information of the nRF8001 in the Hardware Revision String Characteristic
                    lib_aci_set_local_data(&aci_state,
                            PIPE_DEVICE_INFORMATION_HARDWARE_REVISION_STRING_SET,
                            (uint8_t *) &(aci_evt->params.cmd_rsp.params.get_device_version),
                            sizeof(aci_evt_cmd_rsp_params_get_device_version_t));
                }
            break;

            case ACI_EVT_CONNECTED:
                if (debug_ble) printf("Evt Connected\n");
                timing_change_done = false;
                aci_state.data_credit_available = aci_state.data_credit_total;
                aci_state.bonded = true;
                lib_aci_device_version(); // Get the device version of the nRF8001 and store it in the Hardware Revision String
            break;

            case ACI_EVT_PIPE_STATUS:
                if (debug_ble) printf("Evt Pipe Status\n");
                if (!timing_change_done) {
                    // lib_aci_change_timing_GAP_PPCP(); // Pre-programmed parameters from nRFStudio (xml file)
                    if (!lib_aci_change_timing(MIN_INTERVAL, MAX_INTERVAL, SLAVE_LATENCY,
                            SLAVE_TIMEOUT)) {
                        if (debug_ble) printf("Change Timing failed\n");
                    }
                    timing_change_done = true;
                }
            break;

            case ACI_EVT_TIMING:
                if (debug_ble) printf("Evt link connection interval changed\n");
            break;

            case ACI_EVT_DISCONNECTED:
                if (debug_ble) printf("Evt Disconnected/Advertising timed out\n");
                aci_state.bonded = false;
                lib_aci_connect(180/* in seconds */, 0x0100 /* advertising interval 100ms*/);
                if (debug_ble) printf("Advertising started\n");
            break;

            case ACI_EVT_DATA_RECEIVED:
                if (debug_ble) printf("Pipe Number: %d\n", aci_evt->params.data_received.rx_data.pipe_number);
            break;

            case ACI_EVT_DATA_CREDIT:
                aci_state.data_credit_available = aci_state.data_credit_available
                        + aci_evt->params.data_credit.credit;
            break;

            case ACI_EVT_PIPE_ERROR:
                //See the appendix in the nRF8001 Product Specication for details on the error codes
                if (debug_ble) printf("ACI Evt Pipe Error: Pipe #:%d\n", aci_evt->params.pipe_error.pipe_number);
                if (debug_ble) printf("  Pipe Error Code: 0x%x\n", aci_evt->params.pipe_error.error_code);

                //Increment the credit available as the data packet was not sent.
                //The pipe error also represents the Attribute protocol Error Response sent from the peer and that should not be counted
                //for the credit.
                if (ACI_STATUS_ERROR_PEER_ATT_ERROR != aci_evt->params.pipe_error.error_code) {
                    aci_state.data_credit_available++;
                }
            break;

            case ACI_EVT_HW_ERROR:
                if (debug_ble) printf("HW error: %d in ", aci_evt->params.hw_error.line_num);
                for (uint8_t counter = 0; counter <= (aci_evt->len - 3); counter++) {
                    if (debug_ble) printf("%c", aci_evt->params.hw_error.file_name[counter]);
                }
                if (debug_ble) printf("\n");
                lib_aci_connect(180/* in seconds */, 0x0050 /* advertising interval 50ms*/);
                if (debug_ble) printf("Advertising started\n");
            break;

        }
    } else {
        //Serial.println(F("No ACI Events available"));
        // No event in the ACI Event queue and if there is no event in the ACI command queue the arduino can go to sleep
        // Arduino can go to sleep now
        // Wakeup from sleep from the RDYN line
    }

    /* setup_required is set to true when the device starts up and enters setup mode.
     * It indicates that do_aci_setup() should be called. The flag should be cleared if
     * do_aci_setup() returns ACI_STATUS_TRANSACTION_COMPLETE.
     */
    if (setup_required) {
        if (SETUP_SUCCESS == do_aci_setup(&aci_state)) {
            setup_required = false;
        }
    }
}

template<class K> bool sendData(int pipe, const K& data) {
    bool status = false;
    if (aci_state.data_credit_available > 0 && lib_aci_is_pipe_available(&aci_state, pipe)) {
        while (!lib_aci_send_data(pipe, (uint8_t*) &data, sizeof(data))) {
            aci_loop();
        }
        return true;
    }
    return status;
}

