
#include "bootloader.h"
#include "stm32g4xx_hal.h"
#include "sfud.h"



void bootloader_recv_firmware_to_ext_flash(void) {
    // 1. 擦除外挂Flash的固件暂存区（假设从地址0开始，大小64KB）
    sfud_erase(&sfud_dev, 0, 64 * 1024);  
    
    // 2. 通过串口协议（如YModem）接收数据包
    uint32_t recv_offset = 0;
    while (!firmware_recv_done) {
        // 接收一个数据包（包含数据 + CRC）
        // 校验CRC
        // 写入外挂Flash
        sfud_write(&sfud_dev, recv_offset, packet_data, packet_size);
        recv_offset += packet_size;
    }
    
    // 3. 全部接收完成后，计算整体CRC并校验
    uint32_t crc = calculate_crc_for_ext_flash(0, recv_offset); 
    if (crc == expected_crc_from_metadata) {
        // 校验通过，设置升级标志
        write_upgrade_flag(UPGRADE_FLAG_READY);
    } else {
        write_upgrade_flag(UPGRADE_FLAG_FAILED);
    }
}



void bootloader_move_firmware_to_app(void) {
    // 1. 擦除片内 APP 区（从 0x08004000 开始，大小 96KB）
    erase_app_flash(APP_START_ADDR, APP_SIZE);
    
    // 2. 从外挂Flash读一块，写到片内Flash（循环直到全部搬完）
    uint8_t buffer[256];  // 可以用更大的buffer提高效率
    uint32_t ext_addr = 0;
    uint32_t app_addr = APP_START_ADDR;
    
    while (ext_addr < firmware_size) {
        // 从外挂Flash读取
        sfud_read(&sfud_dev, ext_addr, buffer, sizeof(buffer));
        
        // 写入片内Flash
        HAL_FLASH_Unlock();
        for (int i = 0; i < sizeof(buffer); i += 8) {
            // STM32G4 支持双字（64位）编程
            uint64_t data = *(uint64_t*)(buffer + i);
            HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, app_addr + i, data);
        }
        HAL_FLASH_Lock();
        
        ext_addr += sizeof(buffer);
        app_addr += sizeof(buffer);
    }
    
    // 3. 搬运完成，清除升级标志，置“升级成功”标志（可选）
    write_upgrade_flag(UPGRADE_FLAG_IDLE);
}


#define BOOT_TIMEOUT_MS  5000  // 5秒超时

void bootloader_recv_firmware_with_timeout(void) {
    uint32_t start_tick = HAL_GetTick();
    while (!firmware_recv_done) {
        if ((HAL_GetTick() - start_tick) > BOOT_TIMEOUT_MS) {
            // 超时，退出并报错
            write_upgrade_flag(UPGRADE_FLAG_FAILED);
            jump_to_app();  // 跳转到旧APP
            return;
        }
        // 接收数据处理...
    }
}



void bootloader_run_upgrade(void) {
    BootState state = BOOT_STATE_CHECK_UPGRADE;
    
    while (1) {
        switch (state) {
            case BOOT_STATE_CHECK_UPGRADE:
                if (read_upgrade_flag() == UPGRADE_FLAG_READY) {
                    state = BOOT_STATE_MOVE_TO_APP;
                } else if (read_upgrade_flag() == UPGRADE_FLAG_REQUESTED) {
                    state = BOOT_STATE_READY_RECV;
                } else {
                    state = BOOT_STATE_JUMP_TO_APP;
                }
                break;
                
            case BOOT_STATE_READY_RECV:
                // 通知上位机准备发送固件
                send_ack_to_host();
                state = BOOT_STATE_RECV_FIRMWARE;
                break;
                
            case BOOT_STATE_RECV_FIRMWARE:
                bootloader_recv_firmware_to_ext_flash();
                state = BOOT_STATE_VERIFY;
                break;
                
            case BOOT_STATE_VERIFY:
                if (verify_firmware_in_ext_flash()) {
                    state = BOOT_STATE_MOVE_TO_APP;
                } else {
                    state = BOOT_STATE_ERROR;
                }
                break;
                
            case BOOT_STATE_MOVE_TO_APP:
                bootloader_move_firmware_to_app();
                write_upgrade_flag(UPGRADE_FLAG_IDLE);  // 清除升级标志
                state = BOOT_STATE_JUMP_TO_APP;
                break;
                
            case BOOT_STATE_JUMP_TO_APP:
                jump_to_app();
                break;  // 永远不会执行到这里
                
            case BOOT_STATE_ERROR:
                // 错误处理：LED闪烁、打印错误码、等待复位
                handle_error();
                break;
        }
    }
}


void jump_to_app(void) {

    // 1. 获取APP的复位向量（中断向量表的第一个4字节）
    uint32_t app_reset_vector = *(__IO uint32_t*)(APP_START_ADDR + 4);
    
    // 2. 设置主栈指针（中断向量表的第一个4字节）
    __set_MSP(*(__IO uint32_t*)APP_START_ADDR);
    
    // 3. 关闭所有外设中断（防止跳转后中断冲突）
    __disable_irq();
    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF;  // 清除所有中断使能
        NVIC->ICPR[i] = 0xFFFFFFFF;  // 清除所有挂起中断
    }
    __enable_irq();
    
    // 4. 跳转到APP的复位向量
    typedef void (*pFunction)(void);
    pFunction jump_to_app = (pFunction)app_reset_vector;
    jump_to_app();
}

void write_upgrade_flag(uint8_t flag) {
    HAL_FLASH_Unlock();
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD , UPGRADE_FLAG_ADDR, flag);
    HAL_FLASH_Lock();
}

uint8_t read_upgrade_flag(void) {
    return *(__IO uint8_t*)UPGRADE_FLAG_ADDR;
}



bool verify_firmware_in_ext_flash(void) {
    return sfud_verify(&sfud_dev, APP_START_ADDR, firmware_size);
}
