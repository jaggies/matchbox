/*
 * matchbox_sd.c
 *
 *  Created on: Aug 27, 2016
 *      Author: jmiller
 */
#include "handlers.h"
#include "matchbox_sd.h"
#include "util.h"

#define SD_DETECT_PIN                    GPIO_PIN_8
#define SD_DETECT_GPIO_PORT              GPIOB
#define SD_DETECT_IRQn                   EXTI9_5_IRQn

#define SD_SDIO_NVIC_PRIORITY 5 // must be lower than SD_DMA_NVIC_PRIORITY
#define SD_DMA_NVIC_PRIORITY 6
#define SD_DET_NVIC_PRIORITY 7

#define SD_DMAx_Tx_CHANNEL                DMA_CHANNEL_4
#define SD_DMAx_Rx_CHANNEL                DMA_CHANNEL_4
#define SD_DMAx_Tx_STREAM                 DMA2_Stream6
#define SD_DMAx_Rx_STREAM                 DMA2_Stream3
#define SD_DMAx_Tx_IRQn                   DMA2_Stream6_IRQn
#define SD_DMAx_Rx_IRQn                   DMA2_Stream3_IRQn

static const uint32_t SD_IO_TIMEOUT = (uint32_t) -1; // wait long time before reporting timeout
static SD_HandleTypeDef uSdHandle;
static SD_CardInfo uSdCardInfo;

static HAL_SD_ErrorTypedef sdPinInit(void);
static HAL_StatusTypeDef sdDmaInit(void);

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
HAL_StatusTypeDef BSP_SD_Init(void) {
    /* Check if the SD card is plugged in the slot */
    if (!BSP_SD_IsDetected()) {
        return HAL_ERROR;
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

    if (HAL_SD_Init(&uSdHandle, &uSdCardInfo) != HAL_OK) {
       return HAL_ERROR;
    }

    /* Configure SD wide bus width */
    if (HAL_SD_WideBusOperation_Config(&uSdHandle, SDIO_BUS_WIDE_4B) != SD_OK) {
        return HAL_ERROR;
    }

    if (sdPinInit() != HAL_OK) {
       return HAL_ERROR;
    }

    if (sdDmaInit() != HAL_OK) {
       return HAL_ERROR;
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
 * @retval SD_OK or underlying error
 */
HAL_SD_ErrorTypedef BSP_SD_ReadBlocks(uint32_t *data, uint64_t readAddr, uint32_t blockSize,
        uint32_t blockCount) {
    return HAL_SD_ReadBlocks(&uSdHandle, data, readAddr, blockSize, blockCount);
}

/**
 * @brief  Writes block(s) to a specified address in an SD card, in polling mode.
 * @param  data: Pointer to the buffer that will contain the data to transmit
 * @param  writeAddr: Address from where data is to be written
 * @param  blockSize: SD card data block size, that should be 512
 * @param  blockCount: Number of SD blocks to write
 * @retval HAL_OK or underlying error
 */
HAL_SD_ErrorTypedef BSP_SD_WriteBlocks(uint32_t *data, uint64_t writeAddr, uint32_t blockSize,
        uint32_t blockCount) {
    return HAL_SD_WriteBlocks(&uSdHandle, data, writeAddr, blockSize, blockCount);
}

/**
 * @brief  Reads block(s) from a specified address in an SD card in DMA mode.
 * @param  data: Pointer to the buffer that will contain the data to transmit
 * @param  readAddr: Address from where data is to be read
 * @param  blockSize: SD card data block size, that should be 512
 * @param  blockCount: Number of SD blocks to read
 * @retval SD status
 */
HAL_SD_ErrorTypedef BSP_SD_ReadBlocks_DMA(uint32_t *data, uint64_t readAddr, uint32_t blockSize,
        uint32_t blockCount) {
    HAL_SD_ErrorTypedef status;
    if ((status = HAL_SD_ReadBlocks_DMA(&uSdHandle, data, readAddr, blockSize, blockCount))
            != SD_OK) {
        return status;
    }

    // Wait for operation to complete
    return HAL_SD_CheckReadOperation(&uSdHandle, SD_IO_TIMEOUT);
}

/**
 * @brief  Writes block(s) to a specified address in an SD card in DMA mode.
 * @param  data: Pointer to the buffer that will contain the data to transmit
 * @param  writeAddr: Address from where data is to be written
 * @param  blockSize: SD card data block size, that should be 512
 * @param  blockCount: Number of SD blocks to write
 * @retval SD status
 */
HAL_SD_ErrorTypedef BSP_SD_WriteBlocks_DMA(uint32_t *data, uint64_t writeAddr, uint32_t blockSize,
        uint32_t blockCount) {
    HAL_SD_ErrorTypedef status;
    if ((status = HAL_SD_WriteBlocks_DMA(&uSdHandle, data, writeAddr, blockSize, blockCount))
            != SD_OK) {
        return status;
    }

    // Wait for operation to complete
    return HAL_SD_CheckWriteOperation(&uSdHandle, SD_IO_TIMEOUT);
}

/**
 * @brief  Erases the specified memory area of the given SD card.
 * @param  StartAddr: Start byte address
 * @param  EndAddr: End byte address
 * @retval SD status
 */
HAL_SD_ErrorTypedef BSP_SD_Erase(uint64_t startAddr, uint64_t endAddr) {
    return HAL_SD_Erase(&uSdHandle, startAddr, endAddr);
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

    HAL_StatusTypeDef status;

    /* Deinitialize the stream for new transfer */
    if ((status = HAL_DMA_DeInit(&dmaRxHandle)) != HAL_OK) {
        return status;
    }

    /* Configure the DMA stream */
    if ((status = HAL_DMA_Init(&dmaRxHandle)) != HAL_OK) {
        return status;
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
    if ((status = HAL_DMA_DeInit(&dmaTxHandle)) != HAL_OK) {
        return status;
    }

    /* Configure the DMA stream */
    if ((status = HAL_DMA_Init(&dmaTxHandle)) != HAL_OK) {
        return status;
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

HAL_SD_ErrorTypedef sdPinInit(void) {
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

    // try high speed mode
    HAL_SD_ErrorTypedef status;
    if ((status = HAL_SD_HighSpeed(&uSdHandle)) != SD_OK) {
        return status;
    }
    return SD_OK;
}

/**
 * @brief  Gets the current SD card data status.
 * @retval Data transfer state.
 *          This value can be one of the following values:
 *            @arg  SD_TRANSFER_OK: No data transfer is acting
 *            @arg  SD_TRANSFER_BUSY: Data transfer is acting
 *            @arg  SD_TRANSFER_ERROR: Data transfer error
 */
HAL_SD_TransferStateTypedef BSP_SD_GetStatus(void) {
    return (HAL_SD_GetStatus(&uSdHandle));
}

/**
 * @brief  Get SD information about specific SD card.
 * @param  CardInfo: Pointer to HAL_SD_CardInfoTypedef structure
 */
HAL_SD_ErrorTypedef BSP_SD_GetCardInfo(HAL_SD_CardInfoTypedef *CardInfo) {
    /* Get SD card Information */
    return HAL_SD_Get_CardInfo(&uSdHandle, CardInfo);
}
