/**
 * @brief   LED 初始化，闪灯状态控制。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#include <stdatomic.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_strip.h"
#include "usbh_modem_board.h"

#include "app_at.h"
#include "app_gpio.h"
#include "app_gnss.h"
#include "app_ping.h"
#include "app_config.h"

 /**
 * @brief
 */
#define APP_LED_GPIO 38

 /**
 * @brief 日志 TAG。
 */
static const char* TAG = "app_led";

/**
 * @brief 灯条句柄。
 */
static led_strip_handle_t led_strip;

/**
 * @brief 初始化 led 状态值，启动时显示黄色。
 */
static uint32_t app_led_status_array[10][3] = {
    {1, 1, 0},
    {1, 1, 0},
    {1, 1, 0},
    {1, 1, 0},
    {1, 1, 0},
    {1, 1, 0},
    {1, 1, 0},
    {1, 1, 0},
    {1, 1, 0},
    {1, 1, 0},
};

/**
 * @brief 设置红灯闪烁次数。
 * @param num
 */
void app_led_error_num(uint32_t num) {
    for (size_t i = 0; i < num; i++) {
        app_led_status_array[i][1] = 5;
    }
    ESP_LOGI(TAG, "------ LED 状态改变：红色 %lu 次", num);
}

/**
 * @brief 检测网络超时，检查信号强度，当网络超时-->GNSS未移动-->SIM信号正常，重启开发板。
 * @param
 */
static void app_led_check_and_restart(void) {
    int ping_timeout_ts = atomic_load(&app_ping_timeout_ts);
    if (ping_timeout_ts > 0) {
        if (app_gnss_data.valid && app_gnss_data.spd < 2) {// 如果 GSNN 数据有效，并且未移动时，检测信号状态。每小时 3.704 千米视为未移动。
            pthread_mutex_lock(&app_at_data.mutex);
            if (app_at_data.is_csq == false) {// 如果没检测过 CSQ，发送检测命令。
                app_at_send_command("AT+CGNSSPORTSWITCH=0,0\r\n");// 停止 GNSS 数据接收。
                app_at_send_command("AT+CSQ\r\n");// 发送检测信号命令。

            } else {// 发送 CSQ 后，等待 CSQ 返回数据。

                int rssi = app_at_data.rssi;
                int ber = app_at_data.ber;
                if (rssi == 99 || ber == 99 || rssi < 15 || ber > 5) {// 信号未知，或者信号弱，继续接收 GSNN 数据。
                    app_at_data.is_csq = false;// 重置数据，等待下一次循环。
                    app_at_data.rssi = 99;
                    app_at_data.ber = 99;
                    app_at_send_command("AT+CGNSSPORTSWITCH=0,1\r\n");// 开始 GNSS 数据接收。

                } else {
                    esp_restart();// 如果有信号还断网，重启开发板。
                }
            }
            pthread_mutex_unlock(&app_at_data.mutex);
        } else {
            // GNSS 数据无效，或者移动时，什么都不做。
        }
    } else {// PING 返回正常。
        if (app_at_data.is_csq) {// 如果正在接收 CSQ 数据，切换为开始接收 GNSS 数据
            app_at_data.is_csq = false;
            app_at_data.rssi = 99;
            app_at_data.ber = 99;
            app_at_send_command("AT+CGNSSPORTSWITCH=0,1\r\n");// 开始 GNSS 数据接收。
        }
    }
}

/**
 * @brief led 显示任务，10次状态灯，1次蓝色灯。
 */
static void app_led_task(void* param) {
    while (led_strip != NULL) {
        for (int i = 0; i < 10; i++) {
            ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_set_pixel(led_strip, 0, app_led_status_array[i][0], app_led_status_array[i][1], app_led_status_array[i][2]));
            ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_refresh(led_strip));
            vTaskDelay(pdMS_TO_TICKS(500));
            if (i == 9) {
                ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_set_pixel(led_strip, 0, 0, 0, 2));// 蓝色做为间隔符号。
                ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_refresh(led_strip));
                vTaskDelay(pdMS_TO_TICKS(500));
            } else {
                ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_clear(led_strip));
                vTaskDelay(pdMS_TO_TICKS(500));
            }
        }

        app_led_check_and_restart();// 每个周期检测一次网络状态。
    }
}

/**
 * @brief 初始化函数。
 * @param
 */
esp_err_t app_led_init(void) {

    led_strip_config_t strip_config = {
        .strip_gpio_num = APP_LED_GPIO,           // The GPIO that connected to the LED strip's data line
        .max_leds = 1,                            // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812,            // LED strip model
        .flags.invert_out = false,                // whether to invert the output signal
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,    // different clock source can lead to
        .resolution_hz = 10 * 1000 * 1000, // RMT counter clock frequency
        .flags.with_dma = true,            // DMA feature is available on ESP target like ESP32-S3
    };

    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (ret == ESP_OK) {
        xTaskCreate(app_led_task, "app_led_task", 2048, NULL, 10, NULL);// 启动 led 显示任务。
    }
    return ret;
}