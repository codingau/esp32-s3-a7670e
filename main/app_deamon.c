/**
 * @brief   守护线程。
 *
 * @author  nyx
 * @date    2024-07-27
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <pthread.h>
#include <stdatomic.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "app_config.h"
#include "app_at.h"
#include "app_ping.h"
#include "app_mqtt.h"
#include "app_gnss.h"
#include "app_main.h"

 /**
 * @brief 日志 TAG。
 */
static const char* TAG = "app_deamon";

/**
 * @brief APP 当前状态。0 = 启动期间，1 = 运行期间。
 */
int app_status = 0;

/**
 * @brief 发送信号检测 AT 命令，等待返回值。
 */
static void app_deamon_receive_csq(void) {
    app_at_send_command("AT+CGNSSPORTSWITCH=0,0\r\n");// 停止 GNSS 数据接收。
    vTaskDelay(pdMS_TO_TICKS(200));// 等待一下。
    app_at_send_command("AT+AUTOCSQ=1,0\r\n");// 发送检测信号命令。
    atomic_store(&app_at_receive_flag, 1);
}

/**
 * @brief 发送 GNSS 命令，切换 UART 接收数据为 GNSS。
 */
static void app_deamon_receive_gnss(void) {
    app_at_send_command("AT+CGNSSPORTSWITCH=0,1\r\n");// 开始 GNSS 数据接收。
    atomic_store(&app_at_receive_flag, 0);

    pthread_mutex_lock(&app_at_data.mutex);
    app_at_data.rssi = 99;
    app_at_data.ber = 99;
    pthread_mutex_unlock(&app_at_data.mutex);
}

/**
 * @brief 检测网络超时，检查信号强度，当网络超时-->GNSS未移动-->SIM信号正常，重启开发板。
 * @param timeout_flag
 */
static void app_deamon_signal_check_and_restart(int timeout_flag) {

    if (timeout_flag == 0) {
        ESP_LOGW(TAG, "------ PING 返回值：非法！！！");

    } else if (timeout_flag == 1) {
        ESP_LOGI(TAG, "------ PING 返回值：正常。");
        if (atomic_load(&app_at_receive_flag) == 1) {// 如果正在接收 CSQ 数据，切换为开始接收 GNSS 数据。
            app_deamon_receive_gnss();
        }

    } else if (timeout_flag == 2) {
        ESP_LOGE(TAG, "------ PING 返回值：超时！");
        pthread_mutex_lock(&app_gnss_data.mutex);
        bool gnss_valid = app_gnss_data.valid;
        double spd = app_gnss_data.spd;
        pthread_mutex_unlock(&app_gnss_data.mutex);
        ESP_LOGE(TAG, "------ PING 返回值：超时！gnss_valid = %d, spd = %f", gnss_valid, spd);

        if (gnss_valid && spd < 5) {// 如果 GNSS 数据有效，并且未移动时，检测信号状态。
            int receive_flag = atomic_load(&app_at_receive_flag);
            if (receive_flag == 0) {// 如果没检测过 CSQ，发送检测命令。
                ESP_LOGI(TAG, "------ 暂停 GNSS 数据接收，发送 AT+CSQ 命令，检测 4G 信号强度。spd = %f", spd);
                app_deamon_receive_csq();

            } else if (receive_flag == 1) {
                ESP_LOGI(TAG, "------ 已发送 AT+CSQ 命令，等返回值 ......");

            } else if (receive_flag == 2) {// 发送 CSQ 后，等待 CSQ 返回数据。

                pthread_mutex_lock(&app_at_data.mutex);
                int rssi = app_at_data.rssi;
                int ber = app_at_data.ber;
                pthread_mutex_unlock(&app_at_data.mutex);
                ESP_LOGI(TAG, "------ AT+CSQ 命令，接收返回值。rssi = %d, ber = %d", rssi, ber);

                if (rssi == 99 || ber == 99 || rssi < 15 || ber > 5) {// 信号未知，或者信号弱，继续接收 GNSS 数据。
                    ESP_LOGI(TAG, "------ 4G 信号未知，或者信号弱，继续接收 GNSS 数据。");
                    app_deamon_receive_gnss();

                } else {
                    ESP_LOGE(TAG, "------ 4G 信号强度正常，并且是断网状态，重启开发板。执行：esp_restart();");
                    esp_restart();
                }
            }
        } else {
            ESP_LOGE(TAG, "------ GNSS 数据无效，或者移动时，不检测 4G 信号强度。app_gnss_data.valid = %d, app_gnss_data.spd = %f", gnss_valid, spd);

        }
    }
}

/**
 * @brief 守护任务。
 * @param param
 */
static void app_deamon_task(void* param) {
    uint32_t count = 0;
    while (1) {
        if (count % 10 == 0) {
            ESP_LOGI(TAG, "------ 守护任务，执行次数：%lu，APP 状态：%d", count, app_status);
        }

        uint32_t cur_ts = esp_log_timestamp();
        uint32_t loop_last_ts = atomic_load(&loop_last_ts);
        if (cur_ts - loop_last_ts > 60000) {// 大于 60 秒。
            ESP_LOGE(TAG, "------ app_main_loop_task() 执行超时！！！立即执行：esp_restart()");
            esp_restart();
        }

        if (app_status == 1) {

            uint32_t mqtt_last_ts = atomic_load(&app_mqtt_last_ts);
            if (cur_ts - mqtt_last_ts > 10000) {// 大于 10 秒。

                esp_err_t ping_ret = app_ping_start();
                if (ping_ret != ESP_OK) {
                    ESP_LOGE(TAG, "------ PING 函数执行结果：失败！立即执行：esp_restart()");
                    esp_restart();
                }
                int timeout_flag = 0;
                for (int i = 0; i < 11; i++) {
                    vTaskDelay(pdMS_TO_TICKS(100));// 每个循环等待 100 毫秒。
                    timeout_flag = atomic_load(&app_ping_timeout_flag);
                    if (timeout_flag > 0) {// 如果有返回值。
                        break;
                    }
                    if (i == 10) {// 最后一次循环，还没有结果情况，等同于超时。
                        timeout_flag = 2;
                    }
                }
                app_deamon_signal_check_and_restart(timeout_flag);

            } else {// 如果 MQTT 正常，重新接收 GNSS 数据。
                if (atomic_load(&app_at_receive_flag) != 0) {// 如果正在接收 CSQ 数据，切换为开始接收 GNSS 数据。
                    app_deamon_receive_gnss();
                    ESP_LOGI(TAG, "------ MQTT 正常发送，重新开始接收 GNSS 数据。");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));// 每秒执行一次。
        count++;
    }
}

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_deamon_init(void) {
    xTaskCreate(app_deamon_task, "app_deamon_task", 2048, NULL, 1, NULL);// 守护任务，最高等级。
    return ESP_OK;
}