/*
 * fatfs.cpp
 *
 *  Created on: Jun 20, 2016
 *      Author: jmiller
 */

#include <string.h>
#include "matchbox.h"
#include "ff.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "pin.h"

osThreadId defaultTaskHandle;
void StartDefaultTask(void const * argument);

#if defined(DEBUG)
#define debug(a...) printf(a)
#else
#define debug(a...)
#endif

#define error(a...) printf(a)

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

const int SDIO_DET = PB8;

void detectCb(uint32_t pin, void* arg) {
    debug("SD DETECT!\n");
}


void StartDefaultTask(void const * argument) {
    Pin led(LED_PIN, Pin::Config().setMode(Pin::MODE_OUTPUT));
    uint32_t count = 0;
    FIL file = {0};
    FATFS FatFs = {0};   /* Work area (file system object) for logical drive */
    FRESULT status;
    char path[4];

    led.write(0); // off by default

    if (0 == FATFS_LinkDriver(&SD_Driver, &path[0])) {
        if (FR_OK == (status = f_mount(&FatFs, "", 1))) {
            static const char msg[] = "Matchbox was able to write file. Yay!";
            static const char* filename = "matchbox.txt";
            debug("Successfully mounted volume\n");
            if (FR_OK == (status = f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS))) {
                debug("Successfully opened file\n");
                UINT written = 0;
                debug("sizeof(msg)=%d\n", sizeof(msg));
                if (FR_OK != (status = f_write(&file, msg, strlen(msg), &written))) {
                    error("Failed to write %d bytes to %s: status=%d, written=%d\n",
                            sizeof(msg), filename, status, written);
                }
                f_sync(&file);
                f_close(&file);
            } else {
                error("Failed to open file: status=%d\n", status);
            }
            // TODO: unmount?
        } else {
            error("Failed to open volume: status=%d\n", status);
        }
    } else {
        error("Failed to link SD driver\n");
    }

    FIL dataFile = {0};
    if (FR_OK == (status = f_open(&dataFile, "test.dat", FA_WRITE | FA_CREATE_ALWAYS))) {
        debug("Successfully opened data file\n");
        UINT written = 0;
    } else {
        error("Failed to open data file: status=%d\n", status);
    }

    debug("Looping...\n");

    char block[256];
    for (int i = 0; i < sizeof(block); i++) {
        block[i] = i;
    }
    while (1) {
        UINT written = 0;
        if (FR_OK != (status = f_write(&dataFile, block, sizeof(block), &written))) {
            error("Failed to write %d bytes: status=%d, written=%d, count = %d\n",
                    sizeof(block), status, written, count);
            led.write(1); // stuck at on if there's an error
        } else {
            if (!(count%4096)) {
                printf("block %04d\n", count);
                //f_sync(&dataFile);
            }
            led.write(count++ & 1);
        }
        f_sync(&dataFile);
        osDelay(10);
    }
}
