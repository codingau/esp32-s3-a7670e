/**
 * @brief   开发板主函数。
 *
 * @author  nyx
 * @date    2024-07-12
 */
#pragma once

#include <stdbool.h>

 /**
  * @brief 最近一次 LOOP 的时间戳。
  */
extern _Atomic uint32_t app_main_loop_last_ts;

/**
 * @brief 推送 JSON 数据结构。
 */
typedef struct {

    char dev_addr[24];      // 设备地址。
    char dev_time[24];      // 设备时间，毫秒。
    int log_ts;             // 系统启动以后的秒数。
    int ble_ts;             // 最后一次扫描到蓝牙开关的秒数。
    char gpios[128];        // GPIO 电平字符串。

    char gnss_time[24];     // GNSS 时间，毫秒。
    bool gnss_valid;        // 有效性。
    int sat;                // 卫星数。
    double alt;             // 高度，默认单位：M。
    double lat;             // 纬度，十进制。
    double lon;             // 经度，十进制。
    double spd;             // 速度，默认单位：节。
    double trk;             // 航向角度。
    double mag;             // 磁偏角度。

    // 温度。
    // 湿度。
    // 烟雾。
    // 电压。
    int f;                  // 标记是否文件缓存数据。
} app_main_data_t;