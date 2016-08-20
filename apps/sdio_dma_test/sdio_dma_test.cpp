/*
 * sdio_dma_test.cpp
 *
 *  Created on: Jun 20, 2016
 *      Author: jmiller
 */

#define DEBUG

#include <string.h>
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "matchbox.h"
#include "pin.h"
#include "util.h"

#define SD_DMAx_Tx_CHANNEL                DMA_CHANNEL_4
#define SD_DMAx_Rx_CHANNEL                DMA_CHANNEL_4
#define SD_DMAx_TxRx_CLK_ENABLE           __HAL_RCC_DMA2_CLK_ENABLE
#define SD_DMAx_Tx_STREAM                 DMA2_Stream6
#define SD_DMAx_Rx_STREAM                 DMA2_Stream3
#define SD_DMAx_Tx_IRQn                   DMA2_Stream6_IRQn
#define SD_DMAx_Rx_IRQn                   DMA2_Stream3_IRQn

static osThreadId defaultTaskHandle;
static DMA_HandleTypeDef dmaRxHandle;
static DMA_HandleTypeDef dmaTxHandle;
static SD_HandleTypeDef uSdHandle;
static HAL_SD_CardInfoTypedef uSdCardInfo;

void StartDefaultTask(void const * argument);

int main(void) {
    MatchBox* mb = new MatchBox();

    osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 2048);
    defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

    /* Start scheduler */
    osKernelStart();

    /* We should never get here as control is now taken by the scheduler */
    while (1)
        ;
    return 0;
}

bool sdDmaInit(void) {
    bool result = true;
    GPIO_InitTypeDef GPIO_Init_Structure;
    SD_HandleTypeDef *hsd = &uSdHandle;

    /* Enable DMA2 clocks */
    SD_DMAx_TxRx_CLK_ENABLE();

    /* Configure DMA Rx parameters */
    dmaRxHandle.Instance = SD_DMAx_Rx_STREAM;
    dmaRxHandle.Init.Channel = SD_DMAx_Rx_CHANNEL;
    dmaRxHandle.Init.Direction = DMA_PERIPH_TO_MEMORY;
    dmaRxHandle.Init.PeriphInc = DMA_PINC_DISABLE;
    dmaRxHandle.Init.MemInc = DMA_MINC_ENABLE;
    dmaRxHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    dmaRxHandle.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    dmaRxHandle.Init.Mode = DMA_PFCTRL;
    dmaRxHandle.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    dmaRxHandle.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    dmaRxHandle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    dmaRxHandle.Init.MemBurst = DMA_MBURST_INC4;
    dmaRxHandle.Init.PeriphBurst = DMA_PBURST_INC4;

    /* Associate the DMA handle */
    __HAL_LINKDMA(hsd, hdmarx, dmaRxHandle);

    /* Deinitialize the stream for new transfer */
    if (HAL_DMA_DeInit(&dmaRxHandle) != HAL_OK) {
        debug("Failed to DeInit Rx DMA\n");
        result = false;
    }

    /* Configure the DMA stream */
    if (HAL_DMA_Init(&dmaRxHandle) != HAL_OK) {
        debug("Failed to Init Rx DMA\n");
        result = false;
    }

    /* Configure DMA Tx parameters */
    dmaTxHandle.Instance = SD_DMAx_Tx_STREAM;
    dmaTxHandle.Init.Channel = SD_DMAx_Tx_CHANNEL;
    dmaTxHandle.Init.Direction = DMA_MEMORY_TO_PERIPH;
    dmaTxHandle.Init.PeriphInc = DMA_PINC_DISABLE;
    dmaTxHandle.Init.MemInc = DMA_MINC_ENABLE;
    dmaTxHandle.Init.PeriphDataAlignment = DMA_PDATAALIGN_WORD;
    dmaTxHandle.Init.MemDataAlignment = DMA_MDATAALIGN_WORD;
    dmaTxHandle.Init.Mode = DMA_PFCTRL;
    dmaTxHandle.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    dmaTxHandle.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    dmaTxHandle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    dmaTxHandle.Init.MemBurst = DMA_MBURST_INC4;
    dmaTxHandle.Init.PeriphBurst = DMA_PBURST_INC4;

    /* Associate the DMA handle */
    __HAL_LINKDMA(hsd, hdmatx, dmaTxHandle);

    /* Deinitialize the stream for new transfer */
    if (HAL_DMA_DeInit(&dmaTxHandle) != HAL_OK) {
        printf("Failed to DeInit Tx DMA\n");
        result = false;
    }

    /* Configure the DMA stream */
    if (HAL_DMA_Init(&dmaTxHandle) != HAL_OK) {
        printf("Failed to Init Tx DMA\n");
        result = false;
    }

    /* NVIC configuration for DMA transfer complete interrupt */
    HAL_NVIC_SetPriority(SD_DMAx_Rx_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(SD_DMAx_Rx_IRQn);

    /* NVIC configuration for DMA transfer complete interrupt */
    HAL_NVIC_SetPriority(SD_DMAx_Tx_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(SD_DMAx_Tx_IRQn);
    return result;
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
    GPIO_InitTypeDef GPIO_Init_Structure = { 0 };
    GPIO_Init_Structure.Mode = GPIO_MODE_AF_PP;
    GPIO_Init_Structure.Pull = GPIO_PULLUP;
    GPIO_Init_Structure.Speed = GPIO_SPEED_FREQ_MEDIUM;
    GPIO_Init_Structure.Alternate = GPIO_AF12_SDIO;

    /* GPIOC configuration */
    GPIO_Init_Structure.Pin = GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12;
    HAL_GPIO_Init(GPIOC, &GPIO_Init_Structure);

    /* GPIOD configuration */
    GPIO_Init_Structure.Pin = GPIO_PIN_2;
    HAL_GPIO_Init(GPIOD, &GPIO_Init_Structure);

    /* SD Card detect pin configuration */
    GPIO_Init_Structure.Mode = GPIO_MODE_INPUT;
    GPIO_Init_Structure.Pull = GPIO_PULLUP;
    GPIO_Init_Structure.Speed = GPIO_SPEED_LOW;
    GPIO_Init_Structure.Pin = 8;
    HAL_GPIO_Init(GPIOB, &GPIO_Init_Structure);

    /* Initialize SD interface */
    uSdHandle.Instance = SDIO;
    uSdHandle.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
    uSdHandle.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
    uSdHandle.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
    uSdHandle.Init.BusWide = SDIO_BUS_WIDE_1B;
    uSdHandle.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_ENABLE;
    uSdHandle.Init.ClockDiv = 6; //SDIO_TRANSFER_CLK_DIV;

    HAL_SD_ErrorTypedef status;
    if ((status = HAL_SD_Init(&uSdHandle, &uSdCardInfo)) != SD_OK) {
        printf("Failed to init sd: status=%d\n", status);
        return false;
    }

    if ((status = HAL_SD_WideBusOperation_Config(&uSdHandle, SDIO_BUS_WIDE_4B)) != SD_OK) {
        printf("Failed to init wide bus mode, status=%d\n", status);
        return false;
    }

    /* NVIC configuration for SDIO interrupts */
    HAL_NVIC_SetPriority(SDIO_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(SDIO_IRQn);

    // try high speed mode
    if ((status = HAL_SD_HighSpeed(&uSdHandle)) != SD_OK) {
        printf("Failed to set high speed mode, status=%d\n", status);
        while (1)
            ;
    }

    return true;
}

int readBlock(void* data, uint64_t addr) {
    HAL_SD_ErrorTypedef status;
    if ((status = HAL_SD_ReadBlocks(&uSdHandle, (uint32_t*) data, addr, 512, 1)) != SD_OK) {
        printf("Failed to read block: status = %d\n", status);
        return 0;
    }
    return 1;
}

int writeBlock(void* data, uint64_t addr) {
    HAL_SD_ErrorTypedef status;
    if ((status = HAL_SD_WriteBlocks(&uSdHandle, (uint32_t*) data, addr, 512, 1)) != SD_OK) {
        printf("Failed to write block: status = %d\n", status);
        return 0;
    }
    return 1;
}

void dumpBlock(uint8_t* data, int count) {
    for (int i = 0; i < 512; i++) {
        if ((i % 16) == 0) {
            printf("%08x: ", count * 512 + i);
        }
        printf("%02x", data[i]);
        if ((i % 16) == 15) {
            printf(" ");
            for (int j = 15; j > 0; j--) {
                int ch = data[i - j];
                ch = ch < 32 || ch > 127 ? '.' : ch;
                printf("%c", ch);
            }
            printf((i % 16) == 15 ? "\n" : " ");
        }
    }
}

extern "C" int SDGetStatus(SD_HandleTypeDef *hsd);

enum DeathCode {
    SDIO_INIT = 2,
    SDIO_DMA,
    BUFFER_NOT_ALIGNED
};

void blinkOfDeath(Pin& led, int code)
{
    while (1) {
        for (int i = 0; i < code; i++) {
            led.write(1);
            osDelay(125);
            led.write(0);
            osDelay(125);
        }
        osDelay(1000);
    }
}

void StartDefaultTask(void const * argument) {
    Pin led(LED_PIN, Pin::Config().setMode(Pin::MODE_OUTPUT));
    debug("initialize sdio\n");
    if (!sdInit()) {
        debug("Failed to initialize sdio\n");
        blinkOfDeath(led, SDIO_INIT);
    }
    if (!sdDmaInit()) {
        printf("Failed to initialize sdio dma\n");
        blinkOfDeath(led, SDIO_DMA);
    }
    int count = 0;
    uint8_t buff[512];
    if (uint32_t(&buff[0]) & 0x3 != 0) {
        printf("Buffer not aligned!\n");
        blinkOfDeath(led, BUFFER_NOT_ALIGNED);
    }
    bzero(buff, sizeof(buff));

    // Seed with random value
    srand(HAL_GetTick());

    while (1) {
        char tmp[512]; // temporary read buffer, for verification
        for (int i = 0; i < 512; i++) {
            buff[i] = rand() & 0xff;
        }
        writeBlock(&buff[0], count * 512);
        while (0x100 != (SDGetStatus(&uSdHandle) & 0x100))
            ;
        readBlock(&tmp[0], count * 512);
        if (0 != memcmp(tmp, buff, sizeof(buff))) {
            printf("readback error: blocks differ!\n");
        } else {
            if (!(count % 64)) {
                if (count != 0) {
                    printf("\n");
                }
                printf("Block %08x", count);
            } else {
                printf(".");
            }
        }
        //dumpBlock(&buff[0], count);
        led.write(count++ & 1);
    }
}
