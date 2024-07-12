/**
 * @brief   BLE 初始化，蓝牙接近开关功能。
 *
 * @author  nyx
 * @date    2024-07-10
 */
#pragma once

 /**
  * @brief 蓝牙钥匙最后刷新时间。
  */
extern _Atomic int app_ble_disc_ts;

/**
 * @brief 蓝牙开关控制的针脚，是否输出高电平。
 * @param level
 */
void app_ble_gpio_set_level(uint32_t level);

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_ble_init(void);
