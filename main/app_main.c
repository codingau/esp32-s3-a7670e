/**
 * @brief   开发板主函数。
 *
 * @author  nyx
 * @date    2024-07-11
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdatomic.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "cJSON.h"
#include "nmea.h"

#include "app_led.h"
#include "app_sd.h"
#include "app_wifi_ap.h"
#include "app_modem.h"
#include "app_sntp.h"
#include "app_mqtt.h"
#include "app_ble.h"
#include "app_gnss.h"
#include "app_json.h"
#include "app_main.h"
#include "app_config.h"

 /**
 * @brief 日志 TAG。
 */
static const char* TAG = "app_main";

/**
 * @brief 是否写数据到 sd 卡。
 */
static bool is_app_write_sd = false;

/**
 * @brief 是否写数据到网络。
 */
static bool is_app_write_4g = false;

/**
 * @brief 初始化推送数据。
 */
static app_main_data_t cur_data = {
    .dev_addr = "00-00-00-00-00-00",        // 初始全部为 0。
    .dev_time = "1970-01-01T00:00:00.000T", // 初始化为起始时间。
    .log_ts = 0,                            // 初始为 0。
    .ble_ts = 0,                            // 初始为 0。
    .gnss_time = "1970-01-01T00:00:00.000T",// 初始化为起始时间。
    .gnss_valid = false,                    // 有效性为 false。
    .sat = 0,                               // 初始卫星数为 0。
    .alt = 0.0,                             // 初始高度设为 0.0 米。
    .lat = 0.0,                             // 纬度。
    .lon = 0.0,                             // 经度。
    .spd = 0.0,                             // 速度。
    .trk = 0.0,                             // 航向角度。
    .mag_dir = NMEA_CARDINAL_DIR_EAST,      // 磁偏方向。
};

/**
 * @brief 获取当前 UTC 时间字符串，并使用 ISO 8601 标准格式化字符串。
 * @param buffer
 * @param buffer_size
 */
void get_cur_utc_time(char* buffer, size_t buffer_size) {
    struct timeval tv;
    struct tm timeinfo;
    gettimeofday(&tv, NULL);// 获取当前时间，秒和微秒。
    gmtime_r(&tv.tv_sec, &timeinfo); // 转换数据格式，秒的部分。
    strftime(buffer, buffer_size - 1, "%Y-%m-%dT%H:%M:%S", &timeinfo);// 格式化时间，精确到秒。当前的格式是：2024-07-11T02:49:55
    int millisec = tv.tv_usec / 1000;// 计算毫秒。
    snprintf(buffer + strlen(buffer), buffer_size - strlen(buffer) - 1, ".%03dZ", millisec);// 追加毫秒字符串。返回时间格式：2024-07-11T02:49:55.148Z
}

/**
 * @brief 获取 GNSS UTC 时间字符串，并使用 ISO 8601 标准格式化字符串。
 * @param buffer
 */
void get_gnss_utc_time(char* buffer, size_t buffer_size) {
    struct tm timeinfo = app_gnss_data.date_time;
    strftime(buffer, buffer_size, "%Y-%m-%dT%H:%M:%SZ", &timeinfo);// GNSS 时间没有毫秒数。
}

/**
 * @brief 循环任务。
 * @param
 */
void app_main_loop_task(void) {
    get_cur_utc_time(cur_data.dev_time, sizeof(cur_data.dev_time));// 设备时间。
    cur_data.log_ts = esp_log_timestamp() / 1000;// 系统启动以后的秒数。
    cur_data.ble_ts = atomic_load(&app_ble_disc_ts) / 1000;// 最后一次扫描到蓝牙开关的秒数。

    pthread_mutex_lock(&app_gnss_data.mutex);
    get_gnss_utc_time(cur_data.gnss_time, sizeof(cur_data.gnss_time));// GNSS 时间。
    cur_data.gnss_valid = app_gnss_data.valid;// 有效性。
    cur_data.sat = app_gnss_data.sat;// 卫星数。
    cur_data.alt = app_gnss_data.alt;// 高度，默认单位：M。
    cur_data.lat = app_gnss_data.lat;// 纬度。
    cur_data.lon = app_gnss_data.lon;// 经度。
    cur_data.spd = app_gnss_data.spd;// 速度，默认单位：节。
    cur_data.trk = app_gnss_data.trk;// 航向角度。
    cur_data.mag = app_gnss_data.mag;// 磁偏角度。
    pthread_mutex_unlock(&app_gnss_data.mutex);

    if (cur_data.log_ts - cur_data.ble_ts > APP_BLE_LEAVE_TIMEOUT) {// 如果蓝牙开关离开 1 分钟，则关闭。
        app_ble_gpio_set_level(0);
    } else {
        app_ble_gpio_set_level(1);// 如果蓝牙开关在 1 分钟内，则开启。
    }

    char* json = app_json_serialize(&cur_data);

    // 推送数据。
    int ret = app_mqtt_publish(json);
    if (ret < 0) {// 发送失败。
        ESP_LOGI(TAG, "------ ---------------- 发送失败");
    } else {

    }
    cJSON_free(json);
    if (app_sd_log_file != NULL) {
        fsync(fileno(app_sd_log_file));
    }
}

/**
 * @brief 主函数，系统启动，开始循环任务。
 * @param
 */
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
        esp_err_t wifi_ret = app_wifi_ap_init(cur_data.dev_addr);
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

    // 如果 SD 卡可写，创建日志文件。
    if (is_app_write_sd) {
        app_sd_create_log_file();
    }

    // 开启一个无限循环的主任务，每隔 1 秒写一次数据。
    const TickType_t task_period = pdMS_TO_TICKS(1000);  // 1秒
    while (1) {
        TickType_t start_tick = xTaskGetTickCount();// 开始时间。

        app_main_loop_task();// 循环任务。

        TickType_t end_tick = xTaskGetTickCount();// 结束时间。
        TickType_t task_duration = end_tick - start_tick;
        if (task_duration > task_period) {// 是否需要延时至下一个周期。
            vTaskDelay(task_period - (task_duration % task_period));
        } else {
            vTaskDelay(task_period - task_duration);
        }
    }
}
