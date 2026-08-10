/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2016-2018, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: Portable interface for each platform.
 * Created on: 2016-04-23
 */

#include <sfud.h>
#include <stdarg.h>

#include "spi.h"           // CubeMX 生成的 SPI 初始化头文件
#include "gpio.h"          // CubeMX 生成的 GPIO 初始化头文件

// 定义 SPI 句柄（CubeMX 里已经生成了 extern SPI_HandleTypeDef hspi1）
extern SPI_HandleTypeDef hspi1;

// 定义 CS 引脚（CubeMX 里配置的片选引脚）
#define FLASH_CS_GPIO_PORT   GPIOA
#define FLASH_CS_PIN         GPIO_PIN_4   // 改成你实际使用的引脚

static char log_buf[256];

void sfud_log_debug(const char *file, const long line, const char *format, ...);

/**
 * SPI write data then read data
 */
static sfud_err spi_write_read(const sfud_spi *spi, const uint8_t *write_buf, size_t write_size, uint8_t *read_buf,
        size_t read_size) {
    sfud_err result = SFUD_SUCCESS;
    uint8_t send_data, read_data;

    /**
     * add your spi write and read code
     */

// ----- ① 拉低 CS 片选（选中 Flash） -----
    HAL_GPIO_WritePin(FLASH_CS_GPIO_PORT, FLASH_CS_PIN, GPIO_PIN_RESET);

// ----- ② 发送数据（如果有） -----
    if (write_size > 0) {
        if (HAL_SPI_Transmit(&hspi1, (uint8_t *)write_buf, write_size, HAL_MAX_DELAY) != HAL_OK) {
            result = SFUD_ERR_TIMEOUT;
            goto exit;
        }
    }
// ----- ③ 读取数据（如果有） -----
    if (read_size > 0) {
        if (HAL_SPI_Receive(&hspi1, read_buf, read_size, HAL_MAX_DELAY) != HAL_OK) {
            result = SFUD_ERR_TIMEOUT;
            goto exit;
        }
    }
exit:
    // ----- ④ 拉高 CS（释放 Flash） -----
    HAL_GPIO_WritePin(FLASH_CS_GPIO_PORT, FLASH_CS_PIN, GPIO_PIN_SET);

    return result;
}

#ifdef SFUD_USING_QSPI
/**
 * read flash data by QSPI
 */
static sfud_err qspi_read(const struct __sfud_spi *spi, uint32_t addr, sfud_qspi_read_cmd_format *qspi_read_cmd_format,
        uint8_t *read_buf, size_t read_size) {
    sfud_err result = SFUD_SUCCESS;

    /**
     * add your qspi read flash data code
     * 该硬件不支持QSPI模式
     */

    return result;
}
#endif /* SFUD_USING_QSPI */

sfud_err sfud_spi_port_init(sfud_flash *flash) {
    sfud_err result = SFUD_SUCCESS;

    /**
     * add your port spi bus and device object initialize code like this:
     * 
     * 
     * 1. rcc initialize
     * 2. gpio initialize
     * 3. spi device initialize
     * 
    // ----- ① SPI 外设已在 CubeMX 中初始化，这里不再重复 -----
    // CubeMX 已经生成了 MX_SPI1_Init()，在 main() 中已调用
     * 
     * 4. flash->spi and flash->retry item initialize
     *    flash->spi.wr = spi_write_read; //Required
     *    flash->spi.qspi_read = qspi_read; //Required when QSPI mode enable
     *    flash->spi.lock = spi_lock;
     *    flash->spi.unlock = spi_unlock;
     *    flash->spi.user_data = &spix;
     *    flash->retry.delay = null;
     *    flash->retry.times = 10000; //Required
     */

    flash->spi.wr = spi_write_read; //Required
    flash->retry.times = 10000;               // 超时等待次数
    flash->retry.delay = NULL;                // 不使用延时函数


    return result;
}

/**
 * This function is print debug info.
 *
 * @param file the file which has call this function
 * @param line the line number which has call this function
 * @param format output format
 * @param ... args
 */
void sfud_log_debug(const char *file, const long line, const char *format, ...) {
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    printf("[SFUD](%s:%ld) ", file, line);
    /* must use vprintf to print */
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\n", log_buf);
    va_end(args);
}

/**
 * This function is print routine info.
 *
 * @param format output format
 * @param ... args
 */
void sfud_log_info(const char *format, ...) {
    va_list args;

    /* args point to the first variable parameter */
    va_start(args, format);
    printf("[SFUD]");
    /* must use vprintf to print */
    vsnprintf(log_buf, sizeof(log_buf), format, args);
    printf("%s\n", log_buf);
    va_end(args);
}
