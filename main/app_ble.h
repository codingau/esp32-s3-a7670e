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
extern _Atomic int64_t app_ble_disc_time;

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_ble_init(void);
