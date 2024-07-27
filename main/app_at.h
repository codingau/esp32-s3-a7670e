/**
 * @brief   AT 初始化，AT 命令执行模块。
 *
 * @author  nyx
 * @date    2024-07-19
 */
#pragma once

#include <pthread.h>

 /**
  * @brief AT 命令接收标记。
  */
extern _Atomic int app_at_receive_flag;

/**
 * @brief AT 接收数据结构。
 */
typedef struct {
    int rssi;
    int ber;
    pthread_mutex_t mutex;              // 互斥锁。

} app_at_data_t;

/**
 * @brief AT 接收数据。
 */
extern app_at_data_t app_at_data;

/**
 * @brief 发送 AT 命令。
 */
void app_at_send_command(const char* command);

/**
 * @brief 获取信号质量。
 */
void app_at_get_rssi_ber(void);

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_at_init(void);