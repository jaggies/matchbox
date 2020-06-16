/*
 * sdio_test.cpp
 *
 *  Created on: Jun 20, 2016
 *      Author: jmiller
 */

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include "stm32f4xx_hal.h"
#include "matchbox.h"
#include "pin.h"
#include "util.h"

enum DeathCode {
    SDIO_INIT = MatchBox::CODE_LAST, // blink codes start here
    SDIO_DMA,
    SDIO_DMA3,
    SDIO_DMA6,
    SDIO_HS_MODE,
    BUFFER_NOT_ALIGNED
};

#define DMA_NVIC_PRIORITY 6
#define SD_NVIC_PRIORITY 5 // must be lower than DMA_NVIC_PRIORITY
#define SD_DMAx_Tx_CHANNEL                DMA_CHANNEL_4
#define SD_DMAx_Rx_CHANNEL                DMA_CHANNEL_4
#define SD_DMAx_TxRx_CLK_ENABLE           __HAL_RCC_DMA2_CLK_ENABLE
#define SD_DMAx_Tx_STREAM                 DMA2_Stream6
#define SD_DMAx_Rx_STREAM                 DMA2_Stream3
#define SD_DMAx_Tx_IRQn                   DMA2_Stream6_IRQn
#define SD_DMAx_Rx_IRQn                   DMA2_Stream3_IRQn

static osThreadId defaultTaskHandle;
static SD_HandleTypeDef uSdHandle;
static HAL_SD_CardInfoTypeDef uSdCardInfo;
static Pin* led;

void StartDefaultTask(void const * argument);

static const int timeout = -1; // forever

int main(void) {
    MatchBox* mb = new MatchBox();

    osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 1, 2048);
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */
    while (1)
        ;
    return 0;
}

bool sdInit() {
    /* Enable SDIO clock */
    __SDIO_CLK_ENABLE();

    /* Enable GPIOs clock */
    __GPIOC_CLK_ENABLE(); // sd data lines PC8..PC12
    __GPIOD_CLK_ENABLE(); // cmd line D2
    __GPIOB_CLK_ENABLE(); // pin detect

    __HAL_RCC_SDIO_CLK_ENABLE();

    /* Common GPIO configuration */
    GPIO_InitTypeDef GPIO_Init_Structure = {0};
    GPIO_Init_Structure.Mode      = GPIO_MODE_AF_PP;
    GPIO_Init_Structure.Pull      = GPIO_PULLUP;
    GPIO_Init_Structure.Speed     = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_Init_Structure.Alternate = GPIO_AF12_SDIO;

    /* GPIOC configuration */
    GPIO_Init_Structure.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    HAL_GPIO_Init(GPIOC, &GPIO_Init_Structure);

    /* GPIOD configuration */
    GPIO_Init_Structure.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOD, &GPIO_Init_Structure);

    /* SD Card detect pin configuration */
    GPIO_Init_Structure.Mode      = GPIO_MODE_INPUT;
    GPIO_Init_Structure.Pull      = GPIO_PULLUP;
    GPIO_Init_Structure.Speed     = GPIO_SPEED_LOW;
    GPIO_Init_Structure.Pin       = 8;
    HAL_GPIO_Init(GPIOB, &GPIO_Init_Structure);

    /* Initialize SD interface
     * Note HW flow control must be disabled on STM32f415 due to hardware glitches on the SDIOCLK
     * line. See errata:
     *
     * "When enabling the HW flow control by setting bit 14 of the SDIO_CLKCR register to ‘1’,
     * glitches can occur on the SDIOCLK output clock resulting in wrong data to be written
     * into the SD/MMC card or into the SDIO device. As a consequence, a CRC error will be
     * reported to the SD/SDIO MMC host interface (DCRCFAIL bit set to ‘1’ in SDIO_STA register)."
     **/
    uSdHandle.Instance = SDIO;
    uSdHandle.Init.ClockEdge           = SDIO_CLOCK_EDGE_RISING;
    uSdHandle.Init.ClockBypass         = SDIO_CLOCK_BYPASS_DISABLE;
    uSdHandle.Init.ClockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
    uSdHandle.Init.BusWide             = SDIO_BUS_WIDE_1B;
    uSdHandle.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
    uSdHandle.Init.ClockDiv            = SDIO_TRANSFER_CLK_DIV;

    HAL_StatusTypeDef status;
    if((status = HAL_SD_Init(&uSdHandle /*, &uSdCardInfo*/)) != HAL_OK) {
        printf("Failed to initialize SD card: status=%d\n", status);
        return false;
    }

    if((status = HAL_SD_ConfigWideBusOperation(&uSdHandle, SDIO_BUS_WIDE_4B)) != HAL_OK) {
        printf("Failed to init wide bus mode, status=%d\n", status);
        return false;
    }

    /* NVIC configuration for SDIO interrupts */
    HAL_NVIC_SetPriority(SDIO_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(SDIO_IRQn);

    // try high speed mode
//    if ((status = HAL_SD_HighSpeed(&uSdHandle)) != HAL_OK) {
//        printf("Failed to set high speed mode, status=%d\n", status);
//        while(1)
//            ;
//    }

    // enable interrupts for errors (tx underflow)
//    __HAL_SD_SDIO_ENABLE_IT(&uSdHandle, (SDIO_IT_DCRCFAIL |\
//                                    SDIO_IT_DTIMEOUT |\
//                                    SDIO_IT_DATAEND  |\
//                                    SDIO_IT_TXUNDERR |\
//                                    SDIO_IT_STBITERR));
    return true;
}

bool readBlock(void* data, uint32_t block) {
    HAL_StatusTypeDef status;
    debug("%s:\n", __func__);
    while ((status = HAL_SD_ReadBlocks(&uSdHandle, (uint8_t*) data, block, 1, timeout)) == HAL_BUSY) {
        debug("\tbusy\n");
    }

    // Wait for it to complete
    if (status == HAL_OK) {
        while (HAL_SD_GetCardState(&uSdHandle) != HAL_SD_CARD_TRANSFER) {
            osThreadYield();
        }
    }

    if (status != HAL_OK) {
        error("%\tfailed, status = %d\n", status);
        return false;
    }

    debug("\tcomplete\n");
    return true;
}

bool writeBlock(void* data, uint32_t block) {
    HAL_StatusTypeDef status;
    debug("%s:\n", __func__);
    while ((status = HAL_SD_WriteBlocks(&uSdHandle, (uint8_t*) data, block, 1, timeout))== HAL_BUSY) {
        debug("\tbusy\n");
    }

    // Wait for it to complete
    if (status == HAL_OK) {
        while (HAL_SD_GetCardState(&uSdHandle) != HAL_SD_CARD_TRANSFER) {
            osThreadYield();
        }
    }
    if (status != HAL_OK) {
        error("\tfailed, status = %d\n",status);
        return false;
    }

    debug("\tcomplete\n");

    return true;
}

void dumpBlock(uint8_t* data, int block) {
    for (int i = 0; i < 512; i++) {
        if ((i % 16) == 0) {
            debug("%08x: ", block * 512 + i);
        }
        debug("%02x", data[i]);
        if ((i % 16) == 15) {
            debug(" ");
            for (int j = 15; j > 0; j--) {
                int ch = data[i - j];
                ch = ch < 32 || ch > 127 ? '.' : ch;
                printf("%c", ch);
            }
            debug((i % 16) == 15 ? "\n" : " ");
        }
    }
}

void StartDefaultTask(void const * argument) {
    Pin led(LED_PIN, Pin::Config().setMode(Pin::MODE_OUTPUT));
    printf("initialize sdio\n");
    if (!sdInit()) {
        error("Failed to initialize sdio\n");
        MatchBox::blinkOfDeath(led, (MatchBox::BlinkCode) SDIO_INIT);
    }

    uint8_t buff[512];
    if (uint32_t(&buff[0]) & 0x3 != 0) {
        error("Buffer not aligned!\n");
        MatchBox::blinkOfDeath(led, (MatchBox::BlinkCode) BUFFER_NOT_ALIGNED);
    }
    bzero(buff, sizeof(buff));

    // Seed with random value
    srand(HAL_GetTick());

    printf("Starting non-DMA\n");
    debug("RCC_HCLK = %d\n", HAL_RCC_GetHCLKFreq());

    int block = 0;
    while (1) {
        for (int i = 0; i < sizeof(buff); i++) {
            buff[i] = rand() & 0xff;
        }

        while (HAL_SD_GetState(&uSdHandle) != HAL_SD_CARD_READY) {
            debug("%s: card not ready\n", __func__);
        }

        char fail = '.'; // no failure
        if (writeBlock(&buff[0], block)) {
            char tmp[sizeof(buff)]; // temporary read buffer, for verification
            if (readBlock(&tmp[0], block)) {
                if (0 != memcmp(tmp, buff, sizeof(buff))) {
                    fail = 'c'; // failed to compare
                }
            } else {
                fail = 'r'; // failed to read
            }
        } else {
            fail = 'w'; // failed to write
        }

        if (!(block % 64)) {
            printf("\n"); // bug in printf requires a separate call for this
            printf("Block %08x: ", block);
        }

        printf("%c", fail);

        //dumpBlock(&buff[0], count);
        led.write(block++ & 1);
    }
}
