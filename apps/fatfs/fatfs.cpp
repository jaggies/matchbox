/*
 * fatfs.cpp
 *
 *  Created on: Jun 20, 2016
 *      Author: jmiller
 */

#define DEBUG

#include <string.h>
#include <stdlib.h>
#include "matchbox.h"
#include "ff.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "pin.h"
#include "util.h"

osThreadId defaultTaskHandle;
void StartDefaultTask(void const * argument);

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

bool writeFile(const char* fname, Pin& led) {
    bool result = true;
    FIL dataFile = {0};
    FRESULT status;
    if (FR_OK == (status = f_open(&dataFile, fname, FA_WRITE | FA_CREATE_ALWAYS))) {
        debug("Successfully opened data file\n");
        UINT written = 0;
    } else {
        debug("Failed to open data file: status=%d\n", status);
        result = false;
    }

    int count = 0;
    srand(0); // reset random number generator
    while (count < 8192) {
        char block[512];
        UINT written = 0;
        for (int i = 0; i < sizeof(block); i++) {
            block[i] = rand() & 0xff;
        }
        if (FR_OK != (status = f_write(&dataFile, block, sizeof(block), &written))) {
            error("Failed to write %d bytes: status=%d, written=%d, count = %d, ftell=%d\n",
                    sizeof(block), status, written, count, f_tell(&dataFile));
            led.write(1); // stuck at on if there's an error
        } else {
            if (!(count%4096)) {
                printf("block %04d\n", count);
                //f_sync(&dataFile);
            }
        }
        led.write(count++ & 1);
        f_sync(&dataFile);
        osDelay(1);
    }
    if (FR_OK != (status = f_close(&dataFile))) {
        error("Failed to close %s, status=%d\n", fname);
        result = false;;
    }
    return result;
}

bool verifyFile(const char* fname, Pin& led) {
    bool result = true;
    FIL dataFile = {0};
    FRESULT status;
    if (FR_OK == (status = f_open(&dataFile, fname, FA_READ))) {
        debug("Successfully opened data file\n");
        UINT written = 0;
    } else {
        debug("Failed to open data file: status=%d\n", status);
        result = false;
    }

    int count = 0;
    srand(0); // reset random number generator
    while (count < 8192) {
        char reference[512];
        char block[sizeof(reference)];
        UINT bread = 0;
        for (int i = 0; i < sizeof(block); i++) {
            reference[i] = rand() & 0xff;
        }
        if (FR_OK != (status = f_read(&dataFile, block, sizeof(block), &bread))) {
            error("Failed to write %d bytes: status=%d, read=%d, count = %d, ftell=%d\n",
                    sizeof(block), status, bread, count, f_tell(&dataFile));
            led.write(1); // stuck at on if there's an error
            result = false;
            break;
        } else {
            if (memcmp(block, reference, sizeof(reference))) {
                debug("Verify failed at block %d\n", count);
                result = false;
                continue;
            }
            if (!(count%4096)) {
                printf("block %04d\n", count);
            }
        }
        led.write(count++ & 1);
        osDelay(1);
    }
    if (FR_OK != (status = f_close(&dataFile))) {
        debug("Failed to close %s, status=%d\n", fname);
        result = false;;
    }
    return result;
}

enum DeathCode {
    SDIO_MOUNT = MatchBox::CODE_LAST, // blink codes start here
    SDIO_LINK
};

void StartDefaultTask(void const * argument) {
    Pin led(LED_PIN, Pin::Config().setMode(Pin::MODE_OUTPUT));
    FATFS FatFs = {0};   /* Work area (file system object) for logical drive */
    FRESULT status;
    char path[4];

    led.write(0); // off by default

    if (0 == FATFS_LinkDriver(&SD_Driver, &path[0])) {
        if (FR_OK == (status = f_mount(&FatFs, "", 0))) {
            debug("Successfully mounted volume\n");
            printf("Waiting for SD\n");
            int t = 5;
            while (t) {
                printf("%d...\n", t--);
                osDelay(1000);
            }
        } else {
            error("Failed to open volume: status=%d\n", status);
            MatchBox::blinkOfDeath(led, (MatchBox::BlinkCode) SDIO_MOUNT);
        }
    } else {
        error("Failed to link SD driver\n");
        MatchBox::blinkOfDeath(led, (MatchBox::BlinkCode) SDIO_LINK);
    }

    while (1) {
        uint32_t count = 0;
        srand(0); // reset seed so files contain same data

        // Find unused filename
        int i = 0;
        char * fname = NULL;
        while (1) {
            char buff[32];
            FILINFO info = { 0 };
            sprintf(buff, "file%04d.dat", i++);
            if (f_stat(buff, &info) != FR_OK) {
                // file not found, use it
                fname = strndup(buff, sizeof(buff));
                break;
            }
        }

        printf("Writing %s\n", fname);
        if (!writeFile(fname, led)) {
            error("Failed to write file %s\n", fname);
            continue;
        }

        // Verify data...
        printf("Verifying %s\n", fname);
        if (!verifyFile(fname, led)) {
            error("Failed to write file %s\n", fname);
        }

        free(fname);
    }
}
