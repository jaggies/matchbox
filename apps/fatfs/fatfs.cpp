/*
 * fatfs.cpp
 *
 *  Created on: Jun 20, 2016
 *      Author: jmiller
 */

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include "matchbox.h"
#include "matchbox_sd.h" // low-level BSP testing
#include "ff.h"
#include "ff_gen_drv.h"
#include "sd_diskio.h"
#include "pin.h"
#include "util.h"

osThreadId defaultTaskHandle;
void StartDefaultTask(void const * argument);

int main(void) {
    MatchBox* mb = new MatchBox(MatchBox::C168MHz);

    osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 1, 2048);
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

bool writeFile(const char* fname, int blocks, Pin& led) {
    bool result = true;
    FIL dataFile = {0};
    FRESULT status;
    if (FR_OK == (status = f_open(&dataFile, fname, FA_WRITE | FA_CREATE_ALWAYS))) {
        debug("Successfully opened data file\n");
        UINT written = 0;
    } else {
        error("Failed to open data file: status=%d\n", status);
        return false;
    }

    int count = 0;
    srand(0); // reset random number generator
    while (count < blocks) {
        char block[512];
        UINT written = 0;
        for (int i = 0; i < sizeof(block); i++) {
            block[i] = rand() & 0xff;
        }
        if (FR_OK != (status = f_write(&dataFile, block, sizeof(block), &written))) {
            error("Failed to write %d bytes: status=%d, written=%d, count = %d, ftell=%d\n",
                    sizeof(block), status, written, count, f_tell(&dataFile));
            result = false;
            break;
        } else {
            if (!(count%4096)) {
                debug("Block %04d\n", count);
            }
        }
        led.write(count++ & 1);
    }
    if (FR_OK != (status = f_close(&dataFile))) {
        error("Failed to close %s, status=%d\n", fname);
        result = false;;
    }
    return result;
}

bool verifyFile(const char* fname, int blocks, Pin& led) {
    bool result = true;
    FIL dataFile = {0};
    FRESULT status;
    if (FR_OK == (status = f_open(&dataFile, fname, FA_READ))) {
        debug("Successfully opened data file\n");
        UINT written = 0;
    } else {
        error("Failed to open data file: status=%d\n", status);
        return false;
    }

    int count = 0;
    srand(0); // reset random number generator
    while (count < blocks) {
        char reference[512];
        char block[sizeof(reference)];
        UINT bread = 0;
        for (int i = 0; i < sizeof(block); i++) {
            reference[i] = rand() & 0xff;
        }
        if (FR_OK != (status = f_read(&dataFile, block, sizeof(block), &bread))) {
            error("Failed to read %d bytes: status=%d, read=%d, count = %d, ftell=%d\n",
                    sizeof(block), status, bread, count, f_tell(&dataFile));
            led.write(1); // stuck at on if there's an error
            result = false;
            break;
        } else {
            if (memcmp(block, reference, sizeof(reference))) {
                error("Verify failed at block %d\n", count);
                result = false;
                break;
            }
            if (!(count%4096)) {
                debug("verify Block %04d\n", count);
            }
        }
        led.write(count++ & 1);
    }
    if (FR_OK != (status = f_close(&dataFile))) {
        error("Failed to close %s, status=%d\n", fname);
        result = false;;
    }
    return result;
}

enum DeathCode {
    SDIO_MOUNT = MatchBox::CODE_LAST, // blink codes start here
    SDIO_LINK,
    SDIO_LABEL
};

void toggleLed(Pin& led) {
    static int c;
    led.write(c++ & 1);
}

void StartDefaultTask(void const * argument) {
    Pin led(LED_PIN, Pin::Config().setMode(Pin::MODE_OUTPUT));
    FATFS FatFs = {0};   /* Work area (file system object) for logical drive */
    FRESULT status;
    char path[4];

    toggleLed(led); // off by default

    printf("FatFS Test...\n");
    HAL_SD_CardInfoTypeDef info;

    if (!BSP_SD_IsDetected()) {
        printf("Waiting for SD card...\n");
        while (!BSP_SD_IsDetected())
            ;
    }

    BSP_SD_Init();
    if(BSP_SD_GetCardInfo(&info) != MSD_OK) {
        printf("Waiting for card info...\n");
        while (BSP_SD_GetCardInfo(&info) != MSD_OK) {
            printf(".");
            osDelay(500);
        }
    }
    printf("Type: %x\n", info.CardType);
    printf("Block size: %d\n", info.BlockSize);
    printf("Capacity: %dMB\n", info.BlockNbr / (1024*1024 / info.BlockSize)); // Nb * mb/block = mb
    printf("Card type: %d\n", info.CardType);

    if (0 == FATFS_LinkDriver(&SD_Driver, &path[0])) {
        if (FR_OK == (status = f_mount(&FatFs, "", 0))) {
            debug("Successfully mounted volume\n");
            toggleLed(led);
            char label[32];
            // Getting the label has the side effect of mounting the volume
            if (FR_OK == f_getlabel(&path[0], &label[0], 0)) {
                toggleLed(led);
                printf("Opened volume '%s'\n", strlen(label) > 0 ? label : "NONAME");
            } else {
                error("Failed to get label!\n");
                MatchBox::blinkOfDeath(led, (MatchBox::BlinkCode) SDIO_LABEL);
            }
        } else {
            error("Failed to open volume: status=%d\n", status);
            MatchBox::blinkOfDeath(led, (MatchBox::BlinkCode) SDIO_MOUNT);
        }
    } else {
        error("Failed to link SD driver\n");
        MatchBox::blinkOfDeath(led, (MatchBox::BlinkCode) SDIO_LINK);
    }

    toggleLed(led);

    printf("Starting test...\n");
    while (1) {
        uint32_t count = 0;
        srand(0); // reset seed so files contain same data

        // Find unused filename
        char * fname = NULL;
        for (int i = 0; i < 100; i++) {
            char buff[32];
            FILINFO info = { 0 };
            sprintf(buff, "file%04d.dat", i);
            debug("stat %s\n", buff);
            if (f_stat(buff, &info) == FR_OK) {
                debug("Skipping '%s'\n", buff);
            } else {
                // file not found, use it
                fname = strndup(buff, sizeof(buff));
                break;
            }
        }
        if (!fname) {
            printf("Test done. No more files\n");
            while (1)
                ;
        }

        // Write file with random data...
        printf("Writing %s...", fname);
        const int blocks = 8192;
        int startTime = HAL_GetTick();
        if (!writeFile(fname, blocks, led)) {
            printf("failed\n");
            osDelay(1000);
            continue;
        } else {
            int timeNow = HAL_GetTick();
            float dt = float(timeNow - startTime) / 1000.0f;
            printf("done %0.2fkB/s\n", (float) blocks*512 / dt / 1024.0f);
        }

        // Verify data...
        printf("Verifying %s...", fname);
        startTime = HAL_GetTick();
        if (!verifyFile(fname, blocks, led)) {
            printf("failed\n");
            osDelay(1000);
        } else {
            int timeNow = HAL_GetTick();
            float dt = float(timeNow - startTime) / 1000.0f;
            printf("verified %0.2fkB/s\n", (float) blocks*512 / dt / 1024.0f);
        }

        free(fname);
    }
}
