/*
 * hello.cpp
 *
 *  Created on: Jun 20, 2016
 *      Author: jmiller
 */

#include <string.h>
#include <stdlib.h>
#include "stm32f4xx_hal.h"
#include "matchbox.h"
#include "pin.h"

osThreadId defaultTaskHandle;

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

    /* Initialize SD interface */
    uSdHandle.Instance = SDIO;
    uSdHandle.Init.ClockEdge           = SDIO_CLOCK_EDGE_RISING;
    uSdHandle.Init.ClockBypass         = SDIO_CLOCK_BYPASS_DISABLE;
    uSdHandle.Init.ClockPowerSave      = SDIO_CLOCK_POWER_SAVE_DISABLE;
    uSdHandle.Init.BusWide             = SDIO_BUS_WIDE_1B;
    uSdHandle.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_ENABLE;
    uSdHandle.Init.ClockDiv            = 6;//SDIO_TRANSFER_CLK_DIV;

    HAL_SD_ErrorTypedef status;
    if((status = HAL_SD_Init(&uSdHandle, &uSdCardInfo)) != SD_OK) {
        printf("Failed to init sd: status=%d\n", status);
        return false;
    }

    if((status = HAL_SD_WideBusOperation_Config(&uSdHandle, SDIO_BUS_WIDE_4B)) != SD_OK) {
        printf("Failed to init wide bus mode, status=%d\n", status);
        return false;
    }

    /* NVIC configuration for SDIO interrupts */
    HAL_NVIC_SetPriority(SDIO_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(SDIO_IRQn);

    // try high speed mode
    if ((status = HAL_SD_HighSpeed(&uSdHandle)) != SD_OK) {
        printf("Failed to set high speed mode, status=%d\n", status);
        while(1)
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
        if ((i%16) == 0) {
            printf("%08x: ", count * 512 + i);
        }
        printf("%02x", data[i]);
        if ((i%16) == 15) {
            printf(" ");
            for (int j = 15; j > 0; j--) {
                int ch = data[i-j];
                ch = ch < 32 || ch > 127 ? '.':ch;
                printf("%c", ch);
            }
            printf( (i%16) == 15 ? "\n" : " ");
        }
    }
}

extern "C" int SDGetStatus(SD_HandleTypeDef *hsd);

void StartDefaultTask(void const * argument) {
    Pin led(LED_PIN, Pin::Config().setMode(Pin::MODE_OUTPUT));
    printf("initialize sdio\n");
    if (!sdInit()) {
        printf("Failed to initialize sdio\n");
    }
    int count = 0;
    uint8_t buff[512];
    if (uint32_t(&buff[0]) & 0x3 != 0) {
        printf("Buffer not aligned!\n");
        while (1)
            ;
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
//        osDelay(10);
    }
}
