/**
 * @brief   PING 初始化，网络状态检测。
 *
 * @author  nyx
 * @date    2024-07-17
 */
#pragma once

 /**
  * @brief PING 返回 time。
  */
extern _Atomic int app_ping_ret;

/**
 * @brief 开始 PING 网络。
 * @return
 */
esp_err_t app_ping_start();

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_ping_init(void);