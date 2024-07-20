/**
 * @brief   AT 初始化，AT 命令执行模块。
 *
 * @author  nyx
 * @date    2024-07-19
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdatomic.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "app_at.h"
#include "app_gnss.h"
#include "app_config.h"

 /**
 * @brief 日志 TAG。
 */
static const char* TAG = "app_at";

/**
 * @brief 初始化 AT 接收数据。
 */
app_at_data_t app_at_data = {
    .is_csq = false,
    .rssi = 0,
    .ber = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER      // 互斥锁。
};

/**
 * @brief 发送 AT 命令。
 */
void app_at_send_command(const char* command) {
    uart_write_bytes(APP_AT_UART_PORT_NUM, command, strlen(command));
    ESP_LOGI(TAG, "------ AT 发送命令：%s", command);
}

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_at_init(void) {
    uart_config_t uart_config = {
        .baud_rate = APP_AT_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    esp_err_t ret = uart_param_config(APP_AT_UART_PORT_NUM, &uart_config);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = uart_set_pin(APP_AT_UART_PORT_NUM, APP_AT_UART_TX_PIN, APP_AT_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = uart_driver_install(APP_AT_UART_PORT_NUM, APP_AT_UART_BUF_SIZE, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        return ret;
    }
    return ESP_OK;
}