/**
 * @brief   GPIO 初始化，执行模块。
 *
 * @author  nyx
 * @date    2024-07-18
 */
#pragma once

#include "driver/gpio.h"

 /**
  * @brief GPIO 操作函数。
  * @param level
  */
int app_gpio_set_level(gpio_num_t gpio_num, uint32_t level);

/**
 * @brief 电源重置。
 * @param
 */
void app_gpio_power_restart(void);

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_gpio_init(void);