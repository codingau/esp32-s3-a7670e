/**
 * @brief   开发板主函数。
 *
 * @author  nyx
 * @date    2024-07-12
 */
#pragma once

#include <stdbool.h>

 /**
  * @brief 推送 JSON 数据结构。
  */
typedef struct {

    char dev_addr[24];      // 设备地址。
    char dev_time[32];      // 设备时间。
    int log_ts;             // 系统启动以后的数。
    int ble_ts;             // 最后一次扫描到蓝牙开关的数。

    char gnss_time[32];     // GNSS 时间。
    bool gnss_valid;        // 有效性。
    int sat;                // 卫星数。
    double alt;             // 高度，默认单位：M。
    double lat;             // 纬度，十进制。
    double lon;             // 经度，十进制。
    double spd;             // 速度，默认单位：节。
    double trk;             // 航向角度。
    double mag;             // 磁偏角度。
    char mag_dir;           // 磁偏方向。

    // 温度。
    // 湿度。
    // 烟雾。
    // 电压。

} app_main_data_t;