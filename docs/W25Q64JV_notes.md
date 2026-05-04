# W25Q64JV SPI Flash 笔记

## 1. 芯片简介

W25Q64JV 是 Winbond 的串行 NOR Flash，容量为 64Mbit，也就是 8MByte。它通过 SPI 总线访问，适合存放固件、参数、日志、字库、图片资源等掉电不丢失的数据。

当前工程使用的是标准 SPI 模式访问，不使用 Dual SPI、Quad SPI、SFDP、安全寄存器、保护位等高级功能。

常见识别信息：

| 项目 | 值 |
| --- | --- |
| Manufacturer ID | `0xEF` |
| Memory Type | `0x40` |
| Capacity ID | `0x17` |
| JEDEC ID | `EF 40 17` |

## 2. 存储容量与组织

W25Q64JV 总容量为 8MByte，地址范围为 `0x000000` 到 `0x7FFFFF`，使用 24-bit 地址。

| 组织单位 | 大小 | 说明 |
| --- | ---: | --- |
| Page | 256 Byte | 页编程的基本单位 |
| Sector | 4 KByte | 常用最小擦除单位 |
| Block 32K | 32 KByte | 块擦除单位 |
| Block 64K | 64 KByte | 块擦除单位 |
| Chip | 8 MByte | 整片容量 |

重要规则：

- Flash 擦除后每个 bit 为 `1`，读出通常是 `0xFF`。
- 写入只能把 bit 从 `1` 改成 `0`。
- 如果要把 `0` 改回 `1`，必须先擦除对应 sector/block/chip。
- Page Program 单次最多写 256 Byte，不能依赖芯片自动跨页连续写入。

## 3. SPI 硬件连接

当前工程使用 STM32F407 的 `SPI1`：

| W25Q64JV 引脚 | 功能 | STM32F407 当前连接 |
| --- | --- | --- |
| `/CS` | 片选，低有效 | `PA4 / SPI1_CS` |
| `CLK` | SPI 时钟 | `PA5 / SPI1_SCK` |
| `DO / IO1` | SPI MISO | `PA6 / SPI1_MISO` |
| `DI / IO0` | SPI MOSI | `PA7 / SPI1_MOSI` |
| `VCC` | 电源 | 3.3V |
| `GND` | 地 | GND |
| `/WP / IO2` | 写保护/Quad IO | 标准 SPI 下建议拉高 |
| `/HOLD / IO3` | 暂停/Quad IO | 标准 SPI 下建议拉高 |

当前建议使用 SPI Mode 0：

- `CPOL = 0`
- `CPHA = 0`
- MSB first
- `/CS` 空闲高电平，传输期间拉低

如果读 ID 不稳定，先把 SPI 分频降下来。当前工程已从较高频率降到 `SPI_BAUDRATEPRESCALER_16`，便于飞线或面包板调试。

## 4. 常用命令表

| 命令 | 指令码 | 地址字节 | Dummy 字节 | 数据方向 | 说明 |
| --- | ---: | ---: | ---: | --- | --- |
| Write Enable | `0x06` | 0 | 0 | 无 | 写入/擦除前必须发送 |
| Read Status Register-1 | `0x05` | 0 | 0 | Flash -> MCU | 读取 BUSY、WEL 等状态 |
| Read Data | `0x03` | 3 | 0 | Flash -> MCU | 普通读数据 |
| Page Program | `0x02` | 3 | 0 | MCU -> Flash | 页编程，最多 256 Byte |
| Sector Erase 4KB | `0x20` | 3 | 0 | 无 | 擦除 4KB sector |
| Block Erase 32KB | `0x52` | 3 | 0 | 无 | 擦除 32KB block |
| Block Erase 64KB | `0xD8` | 3 | 0 | 无 | 擦除 64KB block |
| Chip Erase | `0xC7` | 0 | 0 | 无 | 擦除整片 |
| Read JEDEC ID | `0x9F` | 0 | 0 | Flash -> MCU | 读取厂商和容量 ID |
| Read Unique ID | `0x4B` | 0 | 4 | Flash -> MCU | 读取 64-bit 唯一 ID |

## 5. 典型读写流程

读取 JEDEC ID：

1. `/CS` 拉低。
2. 发送 `0x9F`。
3. 连续读 3 Byte。
4. `/CS` 拉高。
5. 正常 W25Q64JV 应读到 `EF 40 17`。

读取普通数据：

1. `/CS` 拉低。
2. 发送 `0x03`。
3. 发送 24-bit 地址：`A23..A16`、`A15..A8`、`A7..A0`。
4. 连续读取指定长度数据。
5. `/CS` 拉高。

页写入：

1. 发送 Write Enable：`0x06`。
2. `/CS` 拉低。
3. 发送 Page Program：`0x02`。
4. 发送 24-bit 地址。
5. 发送本页内的数据，最多 256 Byte。
6. `/CS` 拉高。
7. 轮询 Status Register-1 的 BUSY 位，等待写入完成。

擦除 sector/block：

1. 发送 Write Enable：`0x06`。
2. `/CS` 拉低。
3. 发送擦除命令，例如 4KB sector erase 为 `0x20`。
4. 发送 24-bit 地址。
5. `/CS` 拉高。
6. 轮询 BUSY 位，等待擦除完成。

## 6. 页写入注意事项

Page Program 的页大小是 256 Byte。一次 Page Program 如果跨过页边界，芯片不会自动写到下一页，可能出现地址回绕到当前页开头的情况。

所以驱动层应该主动拆分跨页写：

- 计算当前地址距离本页末尾还有多少字节。
- 本次只写本页剩余空间。
- 等待 BUSY 清零。
- 地址移动到下一页后继续写。

当前工程的 `W25Q64JV_Write()` 已经实现跨页拆分。

## 7. 擦除注意事项

Flash 写入前通常要先擦除。推荐优先使用 4KB sector erase，因为它粒度最小，影响范围最可控。

擦除单位：

| 擦除命令 | 大小 | 地址对齐 |
| --- | ---: | --- |
| `0x20` | 4KB | 4KB 边界 |
| `0x52` | 32KB | 32KB 边界 |
| `0xD8` | 64KB | 64KB 边界 |
| `0xC7` | 整片 | 无地址 |

当前驱动允许传入 sector/block 内任意地址，内部会向下对齐到对应擦除边界。

注意：Chip Erase 时间很长，调试时不要频繁调用。

## 8. 状态寄存器要点

当前基础驱动主要使用 Status Register-1：

| Bit | 名称 | 说明 |
| --- | --- | --- |
| bit0 | BUSY | `1` 表示芯片正在写入/擦除，`0` 表示空闲 |
| bit1 | WEL | Write Enable Latch，发送 `0x06` 后置位 |

常用判断：

- 写入或擦除后必须轮询 BUSY。
- BUSY 未清零时不要发起新的写入或擦除。
- 每次写入或擦除前都要重新发送 Write Enable。

## 9. 当前工程使用方式

驱动目录：

```text
Components/W25Q64JV/
├── w25q64jv.c
└── w25q64jv.h
```

当前初始化流程：

```c
W25Q64JV_Status flash_status;

printf("[FLASH] Init W25Q64JV...\r\n");

flash_status = W25Q64JV_Init(&hspi1);
if (flash_status != W25Q64JV_OK) {
    printf("[FLASH] Init failed\r\n");
    Error_Handler();
}
```

常用 API：

```c
W25Q64JV_ReadJedecId(id);
W25Q64JV_ReadUniqueId(uid);
W25Q64JV_Read(address, buffer, length);
W25Q64JV_EraseSector4K(address);
W25Q64JV_Write(address, buffer, length);
```

当前工程还加入了跨页写测试：

- 起始地址：`0x000001F0`
- 长度：`300` Byte
- 会跨过 `0x00000200` 页边界
- 流程：擦除 4KB sector -> 写入 300 Byte -> 读回 -> 逐字节比较

## 10. 常见问题排查

### JEDEC ID 读到 `00 00 00`

优先检查：

- Flash 是否供电正常，VCC 是否为 3.3V。
- GND 是否共地。
- `DO / IO1` 是否正确连接到 `PA6 / SPI1_MISO`。
- `/CS` 是否正确连接到 `PA4`，并且空闲为高电平。
- `/WP / IO2` 和 `/HOLD / IO3` 是否被拉高。
- SPI 频率是否过高，飞线调试建议先降速。

### JEDEC ID 读到 `FF FF FF`

优先检查：

- MISO 是否悬空或被上拉。
- `/CS` 是否没有真正拉低。
- Flash 是否没有焊好，或者芯片没有响应。
- SCK/MOSI 是否接反。

### 写入后读回不一致

优先检查：

- 写入前是否已经擦除。
- 是否跨页写但没有拆分。
- 写入或擦除后是否等待 BUSY 清零。
- 地址是否越界。
- 供电是否稳定。

### 只能把部分 bit 写成 0

这是 NOR Flash 的正常特性。写入只能把 `1` 改成 `0`，如果要恢复为 `1`，必须先擦除。
