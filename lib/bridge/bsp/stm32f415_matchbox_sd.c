/*
 * matchbox_sd.c
 *
 *  Created on: Aug 27, 2016
 *      Author: jmiller
 */
#include <stdio.h>
#include "cmsis_os.h" // osThreadYield()
#include "util.h"
#include "handlers.h"
#include "stm32f415_matchbox_sd.h"
//#include "util.h"

#define SD_DETECT_PIN                    GPIO_PIN_8
#define SD_DETECT_GPIO_PORT              GPIOB
#define SD_DETECT_IRQn                   EXTI9_5_IRQn

#define SD_SDIO_NVIC_PRIORITY 1 // must be lower than SD_DMA_NVIC_PRIORITY
#define SD_DMA_NVIC_PRIORITY 2
#define SD_DET_NVIC_PRIORITY 3

#define SD_DMAx_Tx_CHANNEL                DMA_CHANNEL_4
#define SD_DMAx_Rx_CHANNEL                DMA_CHANNEL_4
#define SD_DMAx_Tx_STREAM                 DMA2_Stream6
#define SD_DMAx_Rx_STREAM                 DMA2_Stream3
#define SD_DMAx_Tx_IRQn                   DMA2_Stream6_IRQn
#define SD_DMAx_Rx_IRQn                   DMA2_Stream3_IRQn

static const uint32_t SD_IO_TIMEOUT = 10000; // wait long time before reporting timeout

static SD_HandleTypeDef uSdHandle;
//static SD_CardInfo uSdCardInfo;

static HAL_StatusTypeDef sdPinInit(void);
static HAL_StatusTypeDef sdDmaInit(void);

extern RTC_HandleTypeDef hrtc;

int32_t get_fattime (void) {
    RTC_TimeTypeDef time_s;
    RTC_DateTypeDef date_s;
    HAL_RTC_GetTime(&hrtc, &time_s, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &date_s, RTC_FORMAT_BIN);
    return (int32_t)(date_s.Date - 80) << 25
         | (int32_t)(date_s.Month + 1) << 21
         | (int32_t)(date_s.Date + 1) << 16
         | (int32_t)(time_s.Hours) << 11
         | (int32_t)(time_s.Minutes) << 5
         | (int32_t)(time_s.Seconds/2);
}

// IRQ handlers.  Declared weak so sdio_dma_test and other apps can override
__weak void SDIO_IRQHandler(void) {
    HAL_SD_IRQHandler(&uSdHandle);
}

__weak void DMA2_Stream6_IRQHandler(void) {
    HAL_DMA_IRQHandler(uSdHandle.hdmatx);
}

__weak void DMA2_Stream3_IRQHandler(void) {
    HAL_DMA_IRQHandler(uSdHandle.hdmarx);
}

/**
 * @brief  Initializes the SD card device.
 * @retval SD status.
 */
uint8_t BSP_SD_Init(void) {
    /* Check if the SD card is plugged in the slot */
    if (!BSP_SD_IsDetected()) {
        return MSD_ERROR;
    }

    /* Enable SDIO and DMA2 clocks */
    __HAL_RCC_SDIO_CLK_ENABLE();
    __DMA2_CLK_ENABLE();

    /* Enable GPIOs clock */
    __GPIOC_CLK_ENABLE(); // sd data lines PC8..PC12
    __GPIOD_CLK_ENABLE(); // cmd line D2
    __GPIOB_CLK_ENABLE(); // pin detect

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
    uSdHandle.Init.ClockEdge = SDIO_CLOCK_EDGE_RISING;
    uSdHandle.Init.ClockBypass = SDIO_CLOCK_BYPASS_DISABLE;
    uSdHandle.Init.ClockPowerSave = SDIO_CLOCK_POWER_SAVE_DISABLE;
    uSdHandle.Init.BusWide = SDIO_BUS_WIDE_1B;
    uSdHandle.Init.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_DISABLE;
    uSdHandle.Init.ClockDiv = SDIO_TRANSFER_CLK_DIV;

    if (HAL_SD_Init(&uSdHandle /*, &uSdCardInfo*/) != HAL_OK) {
       return MSD_ERROR;
    }

    /* Configure SD wide bus width */
    if (HAL_SD_ConfigWideBusOperation(&uSdHandle, SDIO_BUS_WIDE_4B) != HAL_OK) {
        return MSD_ERROR;
    }

    if (sdPinInit() != HAL_OK) {
       return MSD_ERROR;
    }

    if (sdDmaInit() != HAL_OK) {
       return MSD_ERROR;
    }

    return HAL_OK;
}

/**
 * @brief  Configures Interrupt mode for SD detection pin.
 * @retval Returns 0
 */
HAL_StatusTypeDef BSP_SD_ITConfig(void) {
    GPIO_InitTypeDef GPIO_Init_Structure;

    /* Configure Interrupt mode for SD detection pin */
    GPIO_Init_Structure.Mode = GPIO_MODE_IT_RISING_FALLING;
    GPIO_Init_Structure.Pull = GPIO_PULLUP;
    GPIO_Init_Structure.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_Init_Structure.Pin = SD_DETECT_PIN;
    HAL_GPIO_Init(SD_DETECT_GPIO_PORT, &GPIO_Init_Structure);

    /* NVIC configuration for SDIO interrupts */
    HAL_NVIC_SetPriority(SD_DETECT_IRQn, SD_DET_NVIC_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(SD_DETECT_IRQn);

    return HAL_OK;
}

/**
 * @brief  Detects if SD card is correctly plugged in the memory slot or not.
 * @retval Returns 1 if SD is detected
 */
uint8_t BSP_SD_IsDetected(void) {
    return (HAL_GPIO_ReadPin(SD_DETECT_GPIO_PORT, SD_DETECT_PIN) == GPIO_PIN_RESET);
}

/**
 * @brief  Reads block(s) from a specified address in an SD card, in polling mode.
 * @param  data: Pointer to the buffer that will contain the data to transmit
 * @param  readAddr: Address from where data is to be read
 * @param  blockSize: SD card data block size, that should be 512
 * @param  blockCount: Number of SD blocks to read
 * @retval BSP_SD_OK or underlying error
 */
uint8_t BSP_SD_ReadBlocks(uint32_t *data, uint32_t readAddr, uint32_t blockCount, uint32_t timeout) {
    uint8_t status;
    while ( (status = HAL_SD_ReadBlocks(&uSdHandle, (uint8_t*) data, readAddr, blockCount, timeout)
            != HAL_OK) )
        osThreadYield();

    if (status != HAL_OK) {
       debug("Error in %s: status = %d\n", __func__, status);
    }
    return (status != HAL_OK) ? MSD_ERROR : MSD_OK;
}

/**
 * @brief  Writes block(s) to a specified address in an SD card, in polling mode.
 * @param  data: Pointer to the buffer that will contain the data to transmit
 * @param  writeAddr: Address from where data is to be written
 * @param  blockSize: SD card data block size, that should be 512
 * @param  blockCount: Number of SD blocks to write
 * @retval HAL_OK or underlying error
 */
uint8_t BSP_SD_WriteBlocks(uint32_t *data, uint32_t writeAddr, uint32_t blockCount, uint32_t timeout) {
    uint8_t status;
    while ( (status = HAL_SD_WriteBlocks(&uSdHandle, (uint8_t*) data, writeAddr, blockCount, timeout))
            == HAL_BUSY)
        osThreadYield();

    if (status != HAL_OK) {
        debug("Error in %s: status = %d\n", __func__, status);
    }
    return (status != HAL_OK) ? MSD_ERROR : MSD_OK;

}

/**
 * @brief  Reads block(s) from a specified address in an SD card in DMA mode.
 * @param  data: Pointer to the buffer that will contain the data to transmit
 * @param  readAddr: Address from where data is to be read
 * @param  blockSize: SD card data block size, that should be 512
 * @param  blockCount: Number of SD blocks to read
 * @retval SD status
 */
uint8_t BSP_SD_ReadBlocks_DMA(uint32_t *data, uint32_t readAddr, uint32_t blockCount) {
    uint8_t status;
    while ( (status = HAL_SD_ReadBlocks_DMA(&uSdHandle, (uint8_t*) data, readAddr, blockCount))
            == HAL_BUSY)
        osThreadYield();

    if (status != HAL_OK) {
        debug("Error in %s: status = %d\n", __func__, status);
    }
    return (status != HAL_OK) ? MSD_ERROR : MSD_OK;
}

/**
 * @brief  Writes block(s) to a specified address in an SD card in DMA mode.
 * @param  data: Pointer to the buffer that will contain the data to transmit
 * @param  writeAddr: Address from where data is to be written
 * @param  blockSize: SD card data block size, that should be 512
 * @param  blockCount: Number of SD blocks to write
 * @retval SD status
 */
uint8_t BSP_SD_WriteBlocks_DMA(uint32_t *data, uint32_t writeAddr, uint32_t blockCount) {
    uint8_t status;
    while ( (status = HAL_SD_WriteBlocks_DMA(&uSdHandle, (uint8_t*) data, writeAddr, blockCount))
            == HAL_BUSY)
        osThreadYield();

    if (status != HAL_OK) {
        debug("Error in %s: status = %d\n", __func__, status);
    }
    return (status != HAL_OK) ? MSD_ERROR : MSD_OK;;
}

/**
 * @brief  Erases the specified memory area of the given SD card.
 * @param  StartAddr: Start byte address
 * @param  EndAddr: End byte address
 * @retval SD status
 */
uint8_t BSP_SD_Erase(uint32_t startAddr, uint32_t endAddr) {
    return HAL_SD_Erase(&uSdHandle, startAddr, endAddr)
            != HAL_OK ? MSD_ERROR : MSD_OK;
}

HAL_StatusTypeDef sdDmaInit(void) {
    static DMA_HandleTypeDef dmaRxHandle;
    static DMA_HandleTypeDef dmaTxHandle;

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
    __HAL_LINKDMA(&uSdHandle, hdmarx, dmaRxHandle);


    /* Deinitialize the stream for new transfer */
    if (HAL_DMA_DeInit(&dmaRxHandle) != HAL_OK) {
        return MSD_ERROR;
    }

    /* Configure the DMA stream */
    if (HAL_DMA_Init(&dmaRxHandle) != HAL_OK) {
        return MSD_ERROR;
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
    dmaTxHandle.Init.Priority = DMA_PRIORITY_LOW;
    dmaTxHandle.Init.FIFOMode = DMA_FIFOMODE_ENABLE;
    dmaTxHandle.Init.FIFOThreshold = DMA_FIFO_THRESHOLD_FULL;
    dmaTxHandle.Init.MemBurst = DMA_MBURST_INC4;
    dmaTxHandle.Init.PeriphBurst = DMA_PBURST_INC4;

    /* Associate the DMA handle */
    __HAL_LINKDMA(&uSdHandle, hdmatx, dmaTxHandle);

    /* Deinitialize the stream for new transfer */
    if (HAL_DMA_DeInit(&dmaTxHandle) != HAL_OK) {
        return MSD_ERROR;
    }

    /* Configure the DMA stream */
    if (HAL_DMA_Init(&dmaTxHandle) != HAL_OK) {
        return MSD_ERROR;
    }

    /* NVIC configuration for DMA transfer complete interrupt */
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    HAL_NVIC_SetPriority(SD_DMAx_Rx_IRQn, SD_DMA_NVIC_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(SD_DMAx_Rx_IRQn);

    /* NVIC configuration for DMA transfer complete interrupt */
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    HAL_NVIC_SetPriority(SD_DMAx_Tx_IRQn, SD_DMA_NVIC_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(SD_DMAx_Tx_IRQn);

    return HAL_OK;
}

HAL_StatusTypeDef sdPinInit(void) {
    /* Common GPIO configuration */
    GPIO_InitTypeDef GPIO_Init_Structure = { 0 };
    GPIO_Init_Structure.Mode = GPIO_MODE_AF_PP;
    GPIO_Init_Structure.Pull = GPIO_PULLUP;
    GPIO_Init_Structure.Speed = GPIO_SPEED_FREQ_HIGH;
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

    /* NVIC configuration for SDIO interrupts */
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
    HAL_NVIC_SetPriority(SDIO_IRQn, SD_SDIO_NVIC_PRIORITY, 0);
    HAL_NVIC_EnableIRQ(SDIO_IRQn);

    // Try high speed mode. This operation should be followed by the configuration
    // of PLL to have SDIOCK clock between 67 and 75 MHz
//    if (HAL_RCC_GetHCLKFreq() >= 36000000) {
//        HAL_StatusTypeDef status;
//        if ((status = HAL_SD_HighSpeed(&uSdHandle)) != BSP_SD_OK) {
//            return status;
//        }
//    }
    return HAL_OK;
}

/**
 * @brief  Gets the current SD card data status.
 * @retval Data transfer state.
 *          This value can be one of the following values:
 *            @arg  SD_TRANSFER_OK: No data transfer is acting
 *            @arg  SD_TRANSFER_BUSY: Data transfer is acting
 *            @arg  SD_TRANSFER_ERROR: Data transfer error
 */
//HAL_SD_TransferStateTypedef BSP_SD_GetStatus(void) {
//    return (HAL_SD_GetStatus(&uSdHandle));
//}

/**
 * @brief  Get SD information about specific SD card.
 * @param  CardInfo: Pointer to HAL_SD_CardInfoTypeDef structure
 */
uint8_t BSP_SD_GetCardInfo(HAL_SD_CardInfoTypeDef *CardInfo) {
    return HAL_SD_GetCardInfo(&uSdHandle, CardInfo) != HAL_OK ? MSD_ERROR : MSD_OK;
}

/**
  * @brief  Gets the current SD card data status.
  * @retval Data transfer state.
  *          This value can be one of the following values:
  *            @arg  SD_TRANSFER_OK: No data transfer is acting
  *            @arg  SD_TRANSFER_BUSY: Data transfer is acting
  */
uint8_t BSP_SD_GetCardState(void)
{
  return((HAL_SD_GetCardState(&uSdHandle) == HAL_SD_CARD_TRANSFER ) ? SD_TRANSFER_OK : SD_TRANSFER_BUSY);
}

