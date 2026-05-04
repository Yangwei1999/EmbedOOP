#ifndef W25Q64JV_H
#define W25Q64JV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"

typedef enum {
    W25Q64JV_OK = 0,
    W25Q64JV_ERROR,
    W25Q64JV_TIMEOUT,
    W25Q64JV_INVALID_PARAM,
    W25Q64JV_ID_MISMATCH
} W25Q64JV_Status;

#define W25Q64JV_TOTAL_SIZE      (8U * 1024U * 1024U)
#define W25Q64JV_PAGE_SIZE       256U
#define W25Q64JV_SECTOR_SIZE     4096U
#define W25Q64JV_BLOCK32_SIZE    (32U * 1024U)
#define W25Q64JV_BLOCK64_SIZE    (64U * 1024U)

W25Q64JV_Status W25Q64JV_Init(SPI_HandleTypeDef *hspi);
W25Q64JV_Status W25Q64JV_ReadJedecId(uint8_t id[3]);
W25Q64JV_Status W25Q64JV_ReadUniqueId(uint8_t uid[8]);
W25Q64JV_Status W25Q64JV_Read(uint32_t address, uint8_t *data, uint32_t length);
W25Q64JV_Status W25Q64JV_Write(uint32_t address, const uint8_t *data, uint32_t length);
W25Q64JV_Status W25Q64JV_WriteAny(uint32_t address, const uint8_t *data, uint32_t length);
W25Q64JV_Status W25Q64JV_EraseSector4K(uint32_t address);
W25Q64JV_Status W25Q64JV_EraseBlock32K(uint32_t address);
W25Q64JV_Status W25Q64JV_EraseBlock64K(uint32_t address);
W25Q64JV_Status W25Q64JV_EraseChip(void);
W25Q64JV_Status W25Q64JV_IsBusy(uint8_t *busy);

#ifdef __cplusplus
}
#endif

#endif /* W25Q64JV_H */
