/**
 * @brief   GNSS 初始化。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "nmea.h"
#include "gpgga.h"
#include "gprmc.h"

#include "app_gnss.h"

#define APP_GNSS_UART_PORT_NUM      UART_NUM_1
#define APP_GNSS_UART_BAUD_RATE     115200
#define APP_GNSS_UART_TX_PIN        18
#define APP_GNSS_UART_RX_PIN        17
#define APP_GNSS_UART_BUF_SIZE      1024

 /**
 * @brief 日志 TAG。
 */
static const char* TAG = "app_gnss";

/**
 * @brief 初始化 GNSS 数据。
 */
app_gnss_data_t app_gnss_data = {
    .date_time = {0},                   // 初始化为 0，表示未设置具体时间。
    .valid = false,                     // 有效性设为 false
    .sat = 0,                           // 卫星数设为 0
    .alt = 0.0,                         // 高度设为 0.0 米。
    .lat_deg = 0,                       // 纬度度数设为 0
    .lat_min = 0.0,                     // 纬度分数设为 0.0
    .lat_dir = NMEA_CARDINAL_DIR_SOUTH, // 默认纬度方向设为南半球。
    .lon_deg = 0,                       // 经度度数设为 0
    .lon_min = 0.0,                     // 经度分数设为 0.0
    .lon_dir = NMEA_CARDINAL_DIR_EAST,  // 默认经度方向设为东半球。
    .speed = 0.0,                       // 速度设为 0.0 节。
    .trk_deg = 0.0,                     // 航向角度。
    .mag_deg = 0.0,                     // 磁偏角度。
    .mag_dir = NMEA_CARDINAL_DIR_EAST,  // 磁偏方向。
    .mutex = PTHREAD_MUTEX_INITIALIZER  // 初始化互斥锁
};

/**
 * @brief 发送 UART 指令。
 * @param command
 */
static void app_gnss_send_at_command(const char* command) {
    uart_write_bytes(APP_GNSS_UART_PORT_NUM, command, strlen(command));
}

static char s_buf[APP_GNSS_UART_BUF_SIZE + 1];
static size_t s_total_bytes;
static char* s_last_buf_end;

/**
 * @brief 从 UART 读取一行数据。
 * @param out_line_buf
 * @param out_line_len
 * @param timeout_ms
 */
static void app_gnss_uart_read_line(char** out_line_buf, size_t* out_line_len, int timeout_ms) {
    *out_line_buf = NULL;
    *out_line_len = 0;

    if (s_last_buf_end != NULL) {
        /* Data left at the end of the buffer after the last call;
         * copy it to the beginning.
         */
        size_t len_remaining = s_total_bytes - (s_last_buf_end - s_buf);
        memmove(s_buf, s_last_buf_end, len_remaining);
        s_last_buf_end = NULL;
        s_total_bytes = len_remaining;
    }

    /* Read data from the UART */
    int read_bytes = uart_read_bytes(APP_GNSS_UART_PORT_NUM, (uint8_t*)s_buf + s_total_bytes, APP_GNSS_UART_BUF_SIZE - s_total_bytes, pdMS_TO_TICKS(timeout_ms));
    if (read_bytes <= 0) {
        return;
    }

    s_total_bytes += read_bytes;

    /* find start (a dollar sign) */
    char* start = memchr(s_buf, '$', s_total_bytes);

    if (start == NULL) {
        s_total_bytes = 0;
        return;
    }

    /* find end of line */
    char* end = memchr(start, '\r', s_total_bytes - (start - s_buf));
    if (end == NULL || *(++end) != '\n') {
        return;
    }
    end++;

    end[-2] = NMEA_END_CHAR_1;
    end[-1] = NMEA_END_CHAR_2;

    *out_line_buf = start;
    *out_line_len = end - start;
    if (end < s_buf + s_total_bytes) {
        /* some data left at the end of the buffer, record its position until the next call */
        s_last_buf_end = end;
    } else {
        s_total_bytes = 0;
    }
}

/**
 * @brief 循环读取 UART 的任务。
 */
static void app_gnss_uart_task() {

    while (1) {
        char* start;
        size_t length;
        app_gnss_uart_read_line(&start, &length, 100 /* ms */);
        if (length == 0) {
            vTaskDelay(pdMS_TO_TICKS(200));// A7670E 模块当前的输出频率是 1 秒 1 次，每次输出N条记录。
            continue;
        }
        // ESP_LOGW(TAG, "%d ------ %.*s", length, length, start);
        nmea_s* data = nmea_parse(start, length, 0);
        if (data == NULL) {
            ESP_LOGI(TAG, "Failed to parse the sentence!");
            ESP_LOGI(TAG, "Type: %.5s (%d)\n", start + 1, nmea_get_type(start));
            continue;

        }
        if (data->errors != 0) {// 如果有错误，直接丢弃数据，进入下一次循环。
            free(data);
            continue;
        }
        // 只处理 gga 和 rmc，其它类型不需要。
        if (NMEA_GPGGA == data->type) {
            pthread_mutex_lock(&app_gnss_data.mutex);
            nmea_gpgga_s* gga = (nmea_gpgga_s*)data;
            app_gnss_data.sat = gga->n_satellites;
            app_gnss_data.alt = gga->altitude;
            pthread_mutex_unlock(&app_gnss_data.mutex);

        } else if (NMEA_GPRMC == data->type) {
            pthread_mutex_lock(&app_gnss_data.mutex);
            nmea_gprmc_s* rmc = (nmea_gprmc_s*)data;
            app_gnss_data.lat_deg = rmc->latitude.degrees;
            app_gnss_data.lat_min = rmc->latitude.minutes;
            app_gnss_data.lat_dir = rmc->latitude.cardinal;
            app_gnss_data.lon_deg = rmc->longitude.degrees;
            app_gnss_data.lon_min = rmc->longitude.minutes;
            app_gnss_data.lon_dir = rmc->longitude.cardinal;
            app_gnss_data.speed = rmc->gndspd_knots;
            app_gnss_data.trk_deg = rmc->track_deg;
            app_gnss_data.mag_deg = rmc->magvar_deg;
            app_gnss_data.mag_dir = rmc->magvar_cardinal;
            pthread_mutex_unlock(&app_gnss_data.mutex);
        }

        if (data != NULL) {
            free(data);
        }
    }
}

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_gnss_init(void) {

    uart_config_t uart_config = {
        .baud_rate = APP_GNSS_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE };
    // Configure UART driver and pins
    esp_err_t ret = uart_param_config(APP_GNSS_UART_PORT_NUM, &uart_config);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = uart_set_pin(APP_GNSS_UART_PORT_NUM, APP_GNSS_UART_TX_PIN, APP_GNSS_UART_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        return ret;
    }
    ret = uart_driver_install(APP_GNSS_UART_PORT_NUM, APP_GNSS_UART_BUF_SIZE, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        return ret;
    }

    app_gnss_send_at_command("AT+CGNSSPWR=1\r\n");// 上电
    for (int i = 10; i > 0; i--) {
        ESP_LOGI(TAG, "等待 GNSS 模块上电: %d", i);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    app_gnss_send_at_command("AT+CGNSSTST=1\r\n");// 接收数据
    ESP_LOGI(TAG, "设置 GNSS 模块开始接收数据。");
    vTaskDelay(pdMS_TO_TICKS(1000));

    // <parse_data_port> 0 output the parsed data of NMEA to USB AT port. 
    //                   1 output the parsed data of NMEA to UART port. 
    // <nmea_data_port>  0 output raw NMEA data to USB NMEA port. 
    //                   1 output raw NMEA data to UART port.
    app_gnss_send_at_command("AT+CGNSSPORTSWITCH=1,1\r\n");// 实测 parsed 无数据，所以 parsed data 也输出到 uart。避免 usb 上网卡出现未知异常。
    ESP_LOGI(TAG, "切换 GNSS 数据输出端口：UART。");
    vTaskDelay(pdMS_TO_TICKS(1000));// 延迟 1 秒，再进行下一步。

    xTaskCreate(app_gnss_uart_task, "app_gnss_uart_task", 3072, NULL, 8, NULL);// 启动接收任务。
    return ESP_OK;
}