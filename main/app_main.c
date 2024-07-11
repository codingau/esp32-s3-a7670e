#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nmea.h"
#include "app_led.h"
#include "app_sd.h"
#include "app_wifi_ap.h"
#include "app_modem.h"
#include "app_sntp.h"
#include "app_mqtt.h"
#include "app_ble.h"
#include "app_gnss.h"

/**
* @brief 日志 TAG。
*/
static const char* TAG = "app_main";

/**
 * @brief 推送 JSON 数据结构。
 */
typedef struct {

    char dev_addr[18];      // 设备地址。
    char dev_time[24];      // 设备时间。
    int start_time;         // 系统启动以后的秒数。
    int ble_time;           // 最后一次扫描到蓝牙开关的秒数。

    char gnss_time[24];     // GNSS 时间。
    bool valid;             // 有效性。
    int sat;                // 卫星数。
    double alt;             // 高度，默认单位：M。

    int lat_deg;            // 纬度，度。
    double lat_min;         // 纬度，分。
    char lat_dir;           // 纬度方向。

    int lon_deg;            // 经度，度。
    double lon_min;         // 经度，分。
    char lon_dir;           // 经度方向。

    float speed;            // 速度，默认单位：节。

    double trk_deg;         // 航向角度。
    double mag_deg;         // 磁偏角度。
    char mag_dir;           // 磁偏方向。

    // 温度。
    // 湿度。
    // 烟雾。
    // 电压。

} app_main_data_t;

/**
 * @brief 初始化推送数据。
 */
app_main_data_t app_main_data = {
    .dev_addr = "00-00-00-00-00-00",        // 初始全部为 0
    .dev_time = "1970-01-01T00:00:00.000",  // 初始化为起始时间。
    .gnss_time = "1970-01-01T00:00:00.000", // 初始化为起始时间。
    .start_time = 0,                        // 初始为 0。
    .ble_time = 0,                          // 初始为 0。
    .valid = false,                         // 有效性设为 false
    .sat = 0,                               // 卫星数设为 0
    .alt = 0.0,                             // 高度设为 0.0 米。
    .lat_deg = 0,                           // 纬度度数设为 0
    .lat_min = 0.0,                         // 纬度分数设为 0.0
    .lat_dir = NMEA_CARDINAL_DIR_SOUTH,     // 默认纬度方向设为南半球。
    .lon_deg = 0,                           // 经度度数设为 0
    .lon_min = 0.0,                         // 经度分数设为 0.0
    .lon_dir = NMEA_CARDINAL_DIR_EAST,      // 默认经度方向设为东半球。
    .speed = 0.0,                           // 速度设为 0.0 节。
    .trk_deg = 0.0,                         // 航向角度。
    .mag_deg = 0.0,                         // 磁偏角度。
    .mag_dir = NMEA_CARDINAL_DIR_EAST,      // 磁偏方向。
};

static bool is_app_write_sd = false;   // 写数据到 sd 卡。
static bool is_app_write_4g = false;   // 写数据到网络。

void app_main(void) {

    // 初始化 LED，失败不终止运行。
    esp_err_t led_ret = app_led_init();
    if (led_ret != ESP_OK) {// 如果失败，大爷就不闪灯了，其它程序继续运行。
        ESP_LOGE(TAG, "------ 初始化 LED：失败！");
    } else {
        ESP_LOGI(TAG, "------ 初始化 LED：OK。");
    }

    // 初始化 NVS，失败则终止运行。因为其它功能依赖于 NVS。
    esp_err_t nvs_ret = nvs_flash_init();
    if (nvs_ret == ESP_ERR_NVS_NO_FREE_PAGES || nvs_ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {// 如果 NVS 分区空间不足或者发现新版本，需要擦除 NVS 分区并重试初始化。
        nvs_ret = nvs_flash_erase();
        if (nvs_ret != ESP_OK) {
            app_led_error_num(1);// led 红色 n 次。
            ESP_LOGE(TAG, "------ 擦除 NVS：失败！");
            return;
        }
        nvs_ret = nvs_flash_init();
        if (nvs_ret != ESP_OK) {
            app_led_error_num(1);// led 红色 n 次。
            ESP_LOGE(TAG, "------ 初始化 NVS：失败！");
            return;
        }
    } else {
        ESP_LOGI(TAG, "------ 初始化 NVS：OK。");
    }

    // 初始化 SD 卡。
    esp_err_t sd_ret = app_sd_init();
    if (sd_ret != ESP_OK) {// 如果 SD 卡初始化失败，闪灯但不停止工作。
        app_led_error_num(2);// led 红色 n 次。
        is_app_write_sd = false;
        ESP_LOGE(TAG, "------ 初始化 SD 卡：失败！");
    } else {
        is_app_write_sd = true;
        ESP_LOGI(TAG, "------ 初始化 SD 卡：OK。");
    }

    // 初始化事件循环，主要用于网络接口。
    esp_err_t event_loop_ret = esp_event_loop_create_default();
    if (event_loop_ret != ESP_OK) {
        app_led_error_num(3);// led 红色 n 次。
        ESP_LOGE(TAG, "------ 初始化 EVENT_LOOP：失败！");
    } else {
        ESP_LOGI(TAG, "------ 初始化 EVENT_LOOP：OK。");
    }


    // 初始化 NETIF 网络接口。
    esp_err_t netif_ret = ESP_FAIL;
    if (event_loop_ret == ESP_OK) {
        netif_ret = esp_netif_init();
        if (netif_ret != ESP_OK) {
            app_led_error_num(3);// led 红色 n 次。
            ESP_LOGE(TAG, "------ 初始化 NETIF：失败！");
        } else {
            ESP_LOGI(TAG, "------ 初始化 NETIF：OK。");
        }
    }

    // 初始化 WIFI 热点。没有热点还能凑合着跑，热点是为了其它功能提供上网服务，不影响本系统运行。
    if (netif_ret == ESP_OK) {
        esp_err_t wifi_ret = app_wifi_ap_init(app_main_data.dev_addr);
        if (wifi_ret != ESP_OK) {
            app_led_error_num(4);// led 红色 n 次。
            ESP_LOGE(TAG, "------ 初始化 WIFI 热点：失败！");
        } else {
            ESP_LOGI(TAG, "------ 初始化 WIFI 热点：OK。");
        }
    }

    // 初始化 4G MODEM。
    esp_err_t modem_ret = ESP_FAIL;
    if (netif_ret == ESP_OK) {
        modem_ret = app_modem_init();
        if (modem_ret != ESP_OK) {
            app_led_error_num(5);// led 红色 n 次。
            ESP_LOGE(TAG, "------ 初始化 4G MODEM：失败！");
        } else {
            ESP_LOGI(TAG, "------ 初始化 4G MODEM：OK。");
        }
    }

    // 初始化 SNTP。
    esp_err_t sntp_ret = ESP_FAIL;
    if (netif_ret == ESP_OK) {
        sntp_ret = app_sntp_init();
        if (sntp_ret != ESP_OK) {
            app_led_error_num(6);// led 红色 n 次。
            ESP_LOGE(TAG, "------ 初始化 SNTP：失败！");
        } else {
            ESP_LOGI(TAG, "------ 初始化 SNTP：OK。");
        }
    }

    // 初始化 MQTT，失败不终止运行。可以写数据到本地。
    if (modem_ret == ESP_OK) {
        esp_err_t mqtt_ret = app_mqtt_init();
        if (mqtt_ret != ESP_OK) {
            app_led_error_num(7);// led 红色 n 次。
            is_app_write_4g = false;
            ESP_LOGE(TAG, "------ 初始化 MQTT：失败！");
        } else {
            is_app_write_4g = true;
            ESP_LOGI(TAG, "------ 初始化 MQTT：OK。");
        }
    }

    // 初始化 BLE，失败不终止运行。
    esp_err_t ble_ret = app_ble_init();
    if (ble_ret != ESP_OK) {
        app_led_error_num(8);// led 红色 n 次。
        ESP_LOGE(TAG, "------ 初始化 BLE：失败！");
    } else {
        ESP_LOGI(TAG, "------ 初始化 BLE：OK。");
    }

    // GNSS 上电需要时间，所以放到最后执行。
    // 初始化 GNSS，失败不终止运行。没有定位数据也能凑合着跑。
    esp_err_t gnss_ret = app_gnss_init();
    if (gnss_ret != ESP_OK) {
        app_led_error_num(9);// led 红色 n 次。
        ESP_LOGE(TAG, "------ 初始化 GNSS：失败！");
    } else {
        ESP_LOGI(TAG, "------ 初始化 GNSS：OK。");
    }

    // 没有输出目标的情况下，10 秒后重启。
    if (!is_app_write_sd && !is_app_write_4g) {
        vTaskDelay(pdMS_TO_TICKS(10000));
        esp_restart();
        return;// 此行不会被执行，变色的关键字便于代码阅读。
    }

    // 开启一个无限循环的主任务，每隔 1 秒写一次数据。
    // const TickType_t task_period = pdMS_TO_TICKS(1000);  // 1秒
    // while (1) {
    //     TickType_t start_tick = xTaskGetTickCount();// 开始时间。

    //     ESP_LOGI(TAG, "------------------------- %lu", esp_log_timestamp());


    //     vTaskDelay(pdMS_TO_TICKS(500));// 模拟任务执行时间


    //     TickType_t end_tick = xTaskGetTickCount();// 结束时间。
    //     ESP_LOGI(TAG, "Task actual execution time: %lu ticks", end_tick - start_tick);


    //     ESP_LOGI(TAG, "++++++++++++++++++++++ %lu", esp_log_timestamp());

    //     TickType_t task_duration = end_tick - start_tick;
    //     if (task_duration > task_period) {// 是否需要延时至下一个周期。
    //         vTaskDelay(task_period - (task_duration % task_period));
    //     } else {
    //         vTaskDelay(task_period - task_duration);
    //     }
    // }
}
