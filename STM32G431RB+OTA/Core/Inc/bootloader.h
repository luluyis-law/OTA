#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__

#include <stdint.h>

// 硬件地址定义
#define FLASH_BASE_ADDR         0x08000000
#define FLASH_SIZE_KB           128
//#define FLASH_PAGE_SIZE         2048    // STM32G4 每页 2KB,已经在 stm32g4xx_hal_flash.h 中定义

// Bootloader 与 APP 分区地址
#define BOOTLOADER_START_ADDR   (FLASH_BASE_ADDR)
#define BOOTLOADER_SIZE_KB      16

// 升级标志地址 (在 Flash 末尾的 2KB 位置)
#define UPGRADE_FLAG_ADDR       (FLASH_BASE_ADDR + (FLASH_SIZE_KB * 1024) - FLASH_PAGE_SIZE)
#define UPGRADE_FLAG_SIZE       2

// APP 分区地址  需要APP工程配置一致
#define APP_START_ADDR          (FLASH_BASE_ADDR + (BOOTLOADER_SIZE_KB * 1024))
#define APP_SIZE_KB             (FLASH_SIZE_KB - BOOTLOADER_SIZE_KB - UPGRADE_FLAG_SIZE)




// 升级标志值定义
#define UPGRADE_FLAG_IDLE       0x00
#define UPGRADE_FLAG_REQUESTED  0xA5
#define UPGRADE_FLAG_READY      0x5A
#define UPGRADE_FLAG_SUCCESS    0xAA
#define UPGRADE_FLAG_FAILED     0xFF


typedef enum {
    BOOT_STATE_CHECK_UPGRADE,   // 检查升级标志
    BOOT_STATE_READY_RECV,      // 准备接收固件
    BOOT_STATE_RECV_FIRMWARE,   // 接收固件数据（写入外挂Flash）
    BOOT_STATE_VERIFY,          // 校验固件完整性
    BOOT_STATE_MOVE_TO_APP,     // 搬运到片内Flash
    BOOT_STATE_JUMP_TO_APP,     // 跳转到APP
    BOOT_STATE_ERROR            // 错误处理
} BootState;


uint8_t read_upgrade_flag(void);
bool verify_firmware_in_ext_flash(void);
void write_upgrade_flag(uint8_t flag);
void bootloader_run_upgrade(void);
void jump_to_app(void);



#endif
