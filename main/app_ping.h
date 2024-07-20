/**
 * @brief   PING 初始化，网络状态检测。
 *
 * @author  nyx
 * @date    2024-07-17
 */
#pragma once

 /**
  * @brief PING 超时，时间戳。
  */
extern int app_ping_timeout_ts;

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_ping_init(void);