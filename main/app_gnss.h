/**
 * @brief   GNSS 初始化，获取定位数据。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#pragma once

#include <pthread.h>

 /**
  * @brief GNSS 数据结构。
  */
typedef struct {

    struct tm date_time;                // 日期时间。
    bool valid;                         // 有效性。
    int sat;                            // 卫星数。
    double alt;                         // 高度，默认单位：M。
    double lat;                         // 纬度，十进制。
    double lon;                         // 经度，十进制。
    double spd;                         // 速度，默认单位：节。
    double trk;                         // 航向角度。
    double mag;                         // 磁偏角度。
    pthread_mutex_t mutex;              // 互斥锁。

} app_gnss_data_t;

/**
 * @brief GNSS 数据。
 */
extern app_gnss_data_t app_gnss_data;

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_gnss_init(void);
