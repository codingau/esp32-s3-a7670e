/**
 * @brief   GPIO 初始化，执行模块。
 *
 * @author  nyx
 * @date    2024-07-18
 */
#pragma once

#include "driver/gpio.h"

 /**
 * @brief GPIO 操作函数，改变返回 1，未改变返回 0。
  * @param level
  */
int app_gpio_set_level(gpio_num_t gpio_num, uint32_t level);

/**
 * @brief 电源重置，使用 GPIO 控制外部继电器。
 * @param
 */
void app_gpio_power_restart(void);

/**
 * @brief 返回 GPIO 电平字符串。
 * @param buffer
 * @param size
 */
void app_gpio_get_string(char* buffer, size_t size);

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_gpio_init(void);