#pragma once

#include <pthread.h>

/**
 * @brief GNSS 数据结构。
 */
typedef struct {

    struct tm date_time;                // 日期和时间。
    bool valid;                         // 有效性。
    int sat;                            // 卫星数。
    double alt;                         // 高度，默认单位：M。

    int lat_deg;                        // 纬度，度。
    double lat_min;                     // 纬度，分。
    char lat_dir;                       // 纬度方向。

    int lon_deg;                        // 经度，度。
    double lon_min;                     // 经度，分。
    char lon_dir;                       // 经度方向。

    float speed;                        // 速度，默认单位：节。

    double trk_deg;                     // 航向角度。
    double mag_deg;                     // 磁偏角度。
    char mag_dir;                       // 磁偏方向。

    pthread_mutex_t mutex;              // 互斥锁。

} app_gnss_data_t;

esp_err_t app_gnss_init(void);

