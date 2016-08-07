/*
 * fatfs.cpp
 *
 *  Created on: Jun 20, 2016
 *      Author: jmiller
 */

#include "matchbox.h"
#include "ff.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "pin.h"

osThreadId defaultTaskHandle;
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

const int SDIO_DET = PB8;

void detectCb(uint32_t pin, void* arg) {
    printf("SD DETECT!\n");
}


void StartDefaultTask(void const * argument) {
    Pin led(LED_PIN, Pin::Config().setMode(Pin::MODE_OUTPUT));
    uint32_t count = 0;
    FIL file = {0};
    FATFS FatFs = {0};   /* Work area (file system object) for logical drive */
    FRESULT status;
    char path[4];

    if (0 == FATFS_LinkDriver(&SD_Driver, &path[0])) {
        if (FR_OK == (status = f_mount(&FatFs, "", 1))) {
            static const char msg[] = "Matchbox was able to write file. Yay!";
            static const char* filename = "matchbox.txt";
            printf("Successfully mounted volume\n");
            if (FR_OK == (status = f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS))) {
                printf("Successfully opened file\n");
                UINT written = 0;
                printf("sizeof(msg)=%d\n", sizeof(msg));
                if (FR_OK != (status = f_write(&file, msg, strlen(msg), &written))) {
                    printf("Failed to write %d bytes to %s: status=%d, actual=%d\n",
                            sizeof(msg), filename, status, written);
                }
                f_sync(&file);
                f_close(&file);
            } else {
                printf("Failed to open file: status=%d\n", status);
            }
            // TODO: unmount?
        } else {
            printf("Failed to open volume: status=%d\n", status);
        }
    } else {
        printf("Failed to link SD driver\n");
    }

    printf("Looping...\n");

    while (1) {
        led.write(count++ & 1);
        osDelay(250);
    }
}
