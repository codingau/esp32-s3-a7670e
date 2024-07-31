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
#include "mqtt_client.h"
#include "usbh_modem_board.h"

#include "app_config.h"
#include "app_at.h"
#include "app_ping.h"
#include "app_mqtt.h"
#include "app_gnss.h"
#include "app_main.h"
#include "app_modem.h"

 /**
 * @brief 日志 TAG。
 */
static const char* TAG = "app_deamon";

/**
 * @brief APP 当前状态。0 = 启动期间，1 = 运行期间。
 */
int app_status = 0;

/**
 * @brief PPP 停止时间戳。
 */
static int app_deamon_ppp_stop_ts = 0;

/**
 * @brief 检测 SIM 卡状态。
 * @param
 */
static void app_demon_ckeck_sim_state(void) {
    int if_ready = false;
    if (modem_board_get_sim_cart_state(&if_ready) == ESP_OK) {
        if (if_ready == true) {
            ESP_LOGI(TAG, "------ 检测 SIM 卡，状态：正常。");
        } else {
            ESP_LOGW(TAG, "------ 检测 SIM 卡，状态：失败！没有 SIM 卡，或 PIN 错误，延迟 30 秒重启开发板。");
            vTaskDelay(pdMS_TO_TICKS(30000));
            esp_restart();
        }
    } else {
        ESP_LOGE(TAG, "------ 检测 SIM 卡，状态：异常！延迟 30 秒重启开发板。");
        vTaskDelay(pdMS_TO_TICKS(30000));
        esp_restart();
    }
}

/**
 * @brief 检测网络超时，检查信号强度，当网络超时-->GNSS未移动-->SIM信号正常，重启开发板。
 * @param ping_ret
 */
static void app_deamon_check_signal_and_restart_ppp(void) {

    if (app_deamon_ppp_stop_ts == 0) {// 如果上网状态。
        esp_err_t stop_ret = modem_board_ppp_stop(30000);
        if (stop_ret == ESP_OK) {
            app_deamon_ppp_stop_ts = esp_log_timestamp();
            ESP_LOGW(TAG, "------ 停止 4G 上网：成功。");
        } else {
            ESP_LOGE(TAG, "------ 停止 4G 上网：失败！稍后重试：modem_board_ppp_stop()");
        }

    } else {

        if (esp_log_timestamp() - app_deamon_ppp_stop_ts > 600000) {// 断网 10 分钟，直接重启。
            ESP_LOGW(TAG, "------ 停止 4G 上网，超过 10 分钟，重启开发板。立即执行：esp_restart()");
            esp_restart();
        }

        app_demon_ckeck_sim_state();// 先检测 SIM 状态。

        ESP_LOGI(TAG, "------ 发送 AT+CSQ 命令，检测 4G 信号强度。");
        int rssi = 0, ber = 0;
        esp_err_t quality_ret = modem_board_get_signal_quality(&rssi, &ber);
        if (quality_ret == ESP_OK) {
            ESP_LOGI(TAG, "------ AT+CSQ 命令，返回：成功。返回值：rssi = %d, ber = %d", rssi, ber);
            if (rssi == 99 || rssi < 15) {
                ESP_LOGW(TAG, "------ 4G 信号未知，或者信号弱，保持断网状态。");
            } else {
                ESP_LOGW(TAG, "------ 4G 信号强度正常，重启开发板。立即执行：esp_restart()");
                esp_restart();
            }

        } else {
            ESP_LOGE(TAG, "------ AT+CSQ 命令，返回：失败！");
        }
        vTaskDelay(pdMS_TO_TICKS(10000));// 等待 10 秒，降低检测频率。
    }
}

/**
 * @brief 网络状态的守护任务。
 * @param param
 */
static void app_deamon_network_task(void* param) {
    uint32_t count = 0;
    while (1) {
        if (count % 30 == 0) {
            ESP_LOGI(TAG, "------ app_deamon_network_task() 守护任务，执行次数：%lu，APP 状态：%d", count, app_status);
        }

        if (app_status == 1) {
            uint32_t cur_ts = esp_log_timestamp();
            uint32_t mqtt_last_ts = atomic_load(&app_mqtt_last_ts);
            if (cur_ts - mqtt_last_ts > 10000) {// 大于 10 秒。

                if (app_deamon_ppp_stop_ts == 0) {
                    esp_err_t ping_start_ret = app_ping_start();
                    if (ping_start_ret != ESP_OK) {
                        ESP_LOGE(TAG, "------ PING 函数执行结果：失败！立即执行：esp_restart()");
                        esp_restart();
                    }
                    int ping_ret = 0;
                    for (int i = 0; i < 11; i++) {
                        vTaskDelay(pdMS_TO_TICKS(100));// 每个循环等待 100 毫秒。
                        ping_ret = atomic_load(&app_ping_ret);
                        if (ping_ret > 0) {// 如果有返回值。
                            break;
                        }
                        if (i == 10) {// 最后一次循环，还没有结果的情况，等同于超时。
                            ping_ret = -1;
                        }
                    }
                    if (ping_ret == -1) {// 断网状态，直接检测信号。
                        app_deamon_check_signal_and_restart_ppp();
                    } else {
                        ESP_LOGI(TAG, "------ PING 返回 time 值：%d。等待 MQTT 客户端自动重连......", ping_ret);
                        vTaskDelay(pdMS_TO_TICKS(10000));// 等待 10 秒，减少 PING 次数。
                    }

                } else {// 断网状态，直接检测信号。
                    app_deamon_check_signal_and_restart_ppp();
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(1000));// 每秒执行一次。
        count++;
    }
}

/**
 * @brief 主循环任务的守护任务。
 * @param param
 */
static void app_deamon_loop_task(void* param) {
    uint32_t count = 0;
    while (1) {
        ESP_LOGI(TAG, "------ app_deamon_loop_task() 守护任务，执行次数：%lu", count);

        uint32_t cur_ts = esp_log_timestamp();
        uint32_t loop_last_ts = atomic_load(&app_main_loop_last_ts);
        if (cur_ts > 86400000) {// 每天重启一次。
            ESP_LOGE(TAG, "------ app_deamon_loop_task() 定时重启。执行：esp_restart()");
            esp_restart();
        }
        if (cur_ts - loop_last_ts > 60000) {// 大于 60 秒。
            ESP_LOGE(TAG, "------ app_main_loop_task() 执行超时！！！立即执行：esp_restart()");
            esp_restart();
        }

        vTaskDelay(pdMS_TO_TICKS(30000));// 每 30 秒执行一次。
        count++;
    }
}

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_deamon_init(void) {
    xTaskCreate(app_deamon_loop_task, "app_dm_loop_task", 2048, NULL, 1, NULL);// 主循环任务守护任务，优先级 1。
    xTaskCreate(app_deamon_network_task, "app_dm_network_task", 3072, NULL, 2, NULL);// 网络状态守护任务，优先级 2。
    return ESP_OK;
}