#include "w25q64jv.h"

#include "main.h"

#include <string.h>

#define W25Q64JV_CMD_WRITE_ENABLE       0x06U
#define W25Q64JV_CMD_READ_STATUS_REG1   0x05U
#define W25Q64JV_CMD_READ_DATA          0x03U
#define W25Q64JV_CMD_PAGE_PROGRAM       0x02U
#define W25Q64JV_CMD_SECTOR_ERASE_4K    0x20U
#define W25Q64JV_CMD_BLOCK_ERASE_32K    0x52U
#define W25Q64JV_CMD_BLOCK_ERASE_64K    0xD8U
#define W25Q64JV_CMD_CHIP_ERASE         0xC7U
#define W25Q64JV_CMD_READ_JEDEC_ID      0x9FU
#define W25Q64JV_CMD_READ_UNIQUE_ID     0x4BU

#define W25Q64JV_STATUS_BUSY_MASK       0x01U
#define W25Q64JV_MANUFACTURER_ID        0xEFU
#define W25Q64JV_MEMORY_TYPE_ID         0x40U
#define W25Q64JV_CAPACITY_ID            0x17U

#define W25Q64JV_SPI_TIMEOUT_MS         100U
#define W25Q64JV_WRITE_TIMEOUT_MS       500U
#define W25Q64JV_ERASE_4K_TIMEOUT_MS    1000U
#define W25Q64JV_ERASE_32K_TIMEOUT_MS   2000U
#define W25Q64JV_ERASE_64K_TIMEOUT_MS   3000U
#define W25Q64JV_CHIP_ERASE_TIMEOUT_MS  200000U

static SPI_HandleTypeDef *w25q64jv_hspi;
static uint8_t w25q64jv_sector_buffer[W25Q64JV_SECTOR_SIZE];

static void W25Q64JV_Select(void)
{
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_RESET);
}

static void W25Q64JV_Deselect(void)
{
    HAL_GPIO_WritePin(SPI1_CS_GPIO_Port, SPI1_CS_Pin, GPIO_PIN_SET);
}

static W25Q64JV_Status W25Q64JV_CheckReady(void)
{
    if (w25q64jv_hspi == NULL) {
        return W25Q64JV_ERROR;
    }

    return W25Q64JV_OK;
}

static W25Q64JV_Status W25Q64JV_Transmit(const uint8_t *data, uint16_t length)
{
    if (HAL_SPI_Transmit(w25q64jv_hspi, data, length, W25Q64JV_SPI_TIMEOUT_MS) != HAL_OK) {
        return W25Q64JV_ERROR;
    }

    return W25Q64JV_OK;
}

static W25Q64JV_Status W25Q64JV_Receive(uint8_t *data, uint16_t length)
{
    if (HAL_SPI_Receive(w25q64jv_hspi, data, length, W25Q64JV_SPI_TIMEOUT_MS) != HAL_OK) {
        return W25Q64JV_ERROR;
    }

    return W25Q64JV_OK;
}

static W25Q64JV_Status W25Q64JV_ReadStatusReg1(uint8_t *status)
{
    uint8_t command = W25Q64JV_CMD_READ_STATUS_REG1;
    W25Q64JV_Status result;

    if (status == NULL) {
        return W25Q64JV_INVALID_PARAM;
    }

    result = W25Q64JV_CheckReady();
    if (result != W25Q64JV_OK) {
        return result;
    }

    W25Q64JV_Select();
    result = W25Q64JV_Transmit(&command, 1U);
    if (result == W25Q64JV_OK) {
        result = W25Q64JV_Receive(status, 1U);
    }
    W25Q64JV_Deselect();

    return result;
}

static W25Q64JV_Status W25Q64JV_WaitWhileBusy(uint32_t timeout_ms)
{
    uint32_t start_tick = HAL_GetTick();
    uint8_t busy = 1U;
    W25Q64JV_Status result;

    do {
        result = W25Q64JV_IsBusy(&busy);
        if (result != W25Q64JV_OK) {
            return result;
        }

        if (busy == 0U) {
            return W25Q64JV_OK;
        }
    } while ((HAL_GetTick() - start_tick) < timeout_ms);

    return W25Q64JV_TIMEOUT;
}

static W25Q64JV_Status W25Q64JV_WriteEnable(void)
{
    uint8_t command = W25Q64JV_CMD_WRITE_ENABLE;
    W25Q64JV_Status result;

    result = W25Q64JV_CheckReady();
    if (result != W25Q64JV_OK) {
        return result;
    }

    W25Q64JV_Select();
    result = W25Q64JV_Transmit(&command, 1U);
    W25Q64JV_Deselect();

    return result;
}

static W25Q64JV_Status W25Q64JV_SendAddressCommand(uint8_t command, uint32_t address)
{
    uint8_t command_buffer[4];

    command_buffer[0] = command;
    command_buffer[1] = (uint8_t)((address >> 16U) & 0xFFU);
    command_buffer[2] = (uint8_t)((address >> 8U) & 0xFFU);
    command_buffer[3] = (uint8_t)(address & 0xFFU);

    return W25Q64JV_Transmit(command_buffer, sizeof(command_buffer));
}

static W25Q64JV_Status W25Q64JV_CheckRange(uint32_t address, uint32_t length)
{
    if (length == 0U) {
        return W25Q64JV_OK;
    }

    if ((address >= W25Q64JV_TOTAL_SIZE) || (length > (W25Q64JV_TOTAL_SIZE - address))) {
        return W25Q64JV_INVALID_PARAM;
    }

    return W25Q64JV_OK;
}

static W25Q64JV_Status W25Q64JV_Erase(uint8_t command, uint32_t address, uint32_t align_size, uint32_t timeout_ms)
{
    W25Q64JV_Status result;
    uint32_t aligned_address;

    result = W25Q64JV_CheckRange(address, 1U);
    if (result != W25Q64JV_OK) {
        return result;
    }

    result = W25Q64JV_WriteEnable();
    if (result != W25Q64JV_OK) {
        return result;
    }

    aligned_address = address & ~(align_size - 1U);

    W25Q64JV_Select();
    result = W25Q64JV_SendAddressCommand(command, aligned_address);
    W25Q64JV_Deselect();
    if (result != W25Q64JV_OK) {
        return result;
    }

    return W25Q64JV_WaitWhileBusy(timeout_ms);
}

W25Q64JV_Status W25Q64JV_Init(SPI_HandleTypeDef *hspi)
{
    uint8_t id[3];
    W25Q64JV_Status result;

    if (hspi == NULL) {
        return W25Q64JV_INVALID_PARAM;
    }

    w25q64jv_hspi = hspi;
    W25Q64JV_Deselect();
    HAL_Delay(1U);

    result = W25Q64JV_ReadJedecId(id);
    if (result != W25Q64JV_OK) {
        return result;
    }

    if ((id[0] != W25Q64JV_MANUFACTURER_ID) ||
        (id[1] != W25Q64JV_MEMORY_TYPE_ID) ||
        (id[2] != W25Q64JV_CAPACITY_ID)) {
        return W25Q64JV_ID_MISMATCH;
    }

    return W25Q64JV_WaitWhileBusy(W25Q64JV_WRITE_TIMEOUT_MS);
}

W25Q64JV_Status W25Q64JV_ReadJedecId(uint8_t id[3])
{
    uint8_t command = W25Q64JV_CMD_READ_JEDEC_ID;
    W25Q64JV_Status result;

    if (id == NULL) {
        return W25Q64JV_INVALID_PARAM;
    }

    result = W25Q64JV_CheckReady();
    if (result != W25Q64JV_OK) {
        return result;
    }

    W25Q64JV_Select();
    result = W25Q64JV_Transmit(&command, 1U);
    if (result == W25Q64JV_OK) {
        result = W25Q64JV_Receive(id, 3U);
    }
    W25Q64JV_Deselect();

    return result;
}

W25Q64JV_Status W25Q64JV_ReadUniqueId(uint8_t uid[8])
{
    uint8_t command_buffer[5] = {W25Q64JV_CMD_READ_UNIQUE_ID, 0U, 0U, 0U, 0U};
    W25Q64JV_Status result;

    if (uid == NULL) {
        return W25Q64JV_INVALID_PARAM;
    }

    result = W25Q64JV_CheckReady();
    if (result != W25Q64JV_OK) {
        return result;
    }

    W25Q64JV_Select();
    result = W25Q64JV_Transmit(command_buffer, sizeof(command_buffer));
    if (result == W25Q64JV_OK) {
        result = W25Q64JV_Receive(uid, 8U);
    }
    W25Q64JV_Deselect();

    return result;
}

W25Q64JV_Status W25Q64JV_Read(uint32_t address, uint8_t *data, uint32_t length)
{
    W25Q64JV_Status result;

    if ((data == NULL) && (length > 0U)) {
        return W25Q64JV_INVALID_PARAM;
    }

    result = W25Q64JV_CheckRange(address, length);
    if (result != W25Q64JV_OK) {
        return result;
    }

    result = W25Q64JV_CheckReady();
    if ((result != W25Q64JV_OK) || (length == 0U)) {
        return result;
    }

    W25Q64JV_Select();
    result = W25Q64JV_SendAddressCommand(W25Q64JV_CMD_READ_DATA, address);
    while ((result == W25Q64JV_OK) && (length > 0U)) {
        uint16_t chunk = (length > UINT16_MAX) ? UINT16_MAX : (uint16_t)length;

        result = W25Q64JV_Receive(data, chunk);
        data += chunk;
        length -= chunk;
    }
    W25Q64JV_Deselect();

    return result;
}

W25Q64JV_Status W25Q64JV_Write(uint32_t address, const uint8_t *data, uint32_t length)
{
    W25Q64JV_Status result;

    if ((data == NULL) && (length > 0U)) {
        return W25Q64JV_INVALID_PARAM;
    }

    result = W25Q64JV_CheckRange(address, length);
    if (result != W25Q64JV_OK) {
        return result;
    }

    result = W25Q64JV_CheckReady();
    if ((result != W25Q64JV_OK) || (length == 0U)) {
        return result;
    }

    while (length > 0U) {
        uint32_t page_offset = address % W25Q64JV_PAGE_SIZE;
        uint32_t page_remaining = W25Q64JV_PAGE_SIZE - page_offset;
        uint16_t chunk = (length < page_remaining) ? (uint16_t)length : (uint16_t)page_remaining;

        result = W25Q64JV_WriteEnable();
        if (result != W25Q64JV_OK) {
            return result;
        }

        W25Q64JV_Select();
        result = W25Q64JV_SendAddressCommand(W25Q64JV_CMD_PAGE_PROGRAM, address);
        if (result == W25Q64JV_OK) {
            result = W25Q64JV_Transmit(data, chunk);
        }
        W25Q64JV_Deselect();
        if (result != W25Q64JV_OK) {
            return result;
        }

        result = W25Q64JV_WaitWhileBusy(W25Q64JV_WRITE_TIMEOUT_MS);
        if (result != W25Q64JV_OK) {
            return result;
        }

        address += chunk;
        data += chunk;
        length -= chunk;
    }

    return W25Q64JV_OK;
}

W25Q64JV_Status W25Q64JV_WriteAny(uint32_t address, const uint8_t *data, uint32_t length)
{
    W25Q64JV_Status result;

    if ((data == NULL) && (length > 0U)) {
        return W25Q64JV_INVALID_PARAM;
    }

    result = W25Q64JV_CheckRange(address, length);
    if (result != W25Q64JV_OK) {
        return result;
    }

    result = W25Q64JV_CheckReady();
    if ((result != W25Q64JV_OK) || (length == 0U)) {
        return result;
    }

    while (length > 0U) {
        uint32_t sector_address = address & ~(W25Q64JV_SECTOR_SIZE - 1U);
        uint32_t sector_offset = address - sector_address;
        uint32_t sector_remaining = W25Q64JV_SECTOR_SIZE - sector_offset;
        uint32_t chunk = (length < sector_remaining) ? length : sector_remaining;
        uint8_t erase_required = 0U;

        result = W25Q64JV_Read(sector_address, w25q64jv_sector_buffer, W25Q64JV_SECTOR_SIZE);
        if (result != W25Q64JV_OK) {
            return result;
        }

        for (uint32_t i = 0U; i < chunk; i++) {
            uint8_t old_value = w25q64jv_sector_buffer[sector_offset + i];
            uint8_t new_value = data[i];

            if ((old_value & new_value) != new_value) {
                erase_required = 1U;
                break;
            }
        }

        if (erase_required != 0U) {
            memcpy(&w25q64jv_sector_buffer[sector_offset], data, chunk);

            result = W25Q64JV_EraseSector4K(sector_address);
            if (result != W25Q64JV_OK) {
                return result;
            }

            result = W25Q64JV_Write(sector_address, w25q64jv_sector_buffer, W25Q64JV_SECTOR_SIZE);
        } else {
            result = W25Q64JV_Write(address, data, chunk);
        }

        if (result != W25Q64JV_OK) {
            return result;
        }

        address += chunk;
        data += chunk;
        length -= chunk;
    }

    return W25Q64JV_OK;
}

W25Q64JV_Status W25Q64JV_EraseSector4K(uint32_t address)
{
    return W25Q64JV_Erase(W25Q64JV_CMD_SECTOR_ERASE_4K, address, W25Q64JV_SECTOR_SIZE, W25Q64JV_ERASE_4K_TIMEOUT_MS);
}

W25Q64JV_Status W25Q64JV_EraseBlock32K(uint32_t address)
{
    return W25Q64JV_Erase(W25Q64JV_CMD_BLOCK_ERASE_32K, address, W25Q64JV_BLOCK32_SIZE, W25Q64JV_ERASE_32K_TIMEOUT_MS);
}

W25Q64JV_Status W25Q64JV_EraseBlock64K(uint32_t address)
{
    return W25Q64JV_Erase(W25Q64JV_CMD_BLOCK_ERASE_64K, address, W25Q64JV_BLOCK64_SIZE, W25Q64JV_ERASE_64K_TIMEOUT_MS);
}

W25Q64JV_Status W25Q64JV_EraseChip(void)
{
    W25Q64JV_Status result;
    uint8_t command = W25Q64JV_CMD_CHIP_ERASE;

    result = W25Q64JV_WriteEnable();
    if (result != W25Q64JV_OK) {
        return result;
    }

    W25Q64JV_Select();
    result = W25Q64JV_Transmit(&command, 1U);
    W25Q64JV_Deselect();
    if (result != W25Q64JV_OK) {
        return result;
    }

    return W25Q64JV_WaitWhileBusy(W25Q64JV_CHIP_ERASE_TIMEOUT_MS);
}

W25Q64JV_Status W25Q64JV_IsBusy(uint8_t *busy)
{
    uint8_t status;
    W25Q64JV_Status result;

    if (busy == NULL) {
        return W25Q64JV_INVALID_PARAM;
    }

    result = W25Q64JV_ReadStatusReg1(&status);
    if (result != W25Q64JV_OK) {
        return result;
    }

    *busy = ((status & W25Q64JV_STATUS_BUSY_MASK) != 0U) ? 1U : 0U;

    return W25Q64JV_OK;
}
