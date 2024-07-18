/**
 * @brief   MODEM 初始化，上网功能。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "usbh_modem_board.h"
#include "usbh_modem_wifi.h"

#include "app_gpio.h"
#include "app_config.h"

 /**
 * @brief 日志 TAG。
 */
static const char* TAG = "app_modem";

/**
 * @brief 猫的事件回调函数。
 * @param arg
 * @param event_base
 * @param event_id
 * @param event_data
 */
static void app_modem_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == MODEM_BOARD_EVENT) {
        if (event_id == MODEM_EVENT_DTE_CONN) {
            ESP_LOGI(TAG, "------ Modem Board Event: USB 已连接。");

        } else if (event_id == MODEM_EVENT_DTE_DISCONN) {
            ESP_LOGW(TAG, "------ Modem Board Event: USB 已断开！");

        } else if (event_id == MODEM_EVENT_SIMCARD_CONN) {
            ESP_LOGI(TAG, "------ Modem Board Event: SIM Card 已连接。");

        } else if (event_id == MODEM_EVENT_SIMCARD_DISCONN) {
            ESP_LOGW(TAG, "------ Modem Board Event: SIM Card 已断开！");

        } else if (event_id == MODEM_EVENT_DTE_RESTART) {
            ESP_LOGW(TAG, "------ Modem Board Event: 硬件重启。");

        } else if (event_id == MODEM_EVENT_DTE_RESTART_DONE) {
            ESP_LOGI(TAG, "------ Modem Board Event: 硬件重启完成。");

        } else if (event_id == MODEM_EVENT_NET_CONN) {
            ESP_LOGI(TAG, "------ Modem Board Event: 网络连接。");

        } else if (event_id == MODEM_EVENT_NET_DISCONN) {
            ESP_LOGW(TAG, "------ Modem Board Event: 网络断开！");

        } else if (event_id == MODEM_EVENT_WIFI_STA_CONN) {
            ESP_LOGI(TAG, "------ Modem Board Event: 热点加入设备。");

        } else if (event_id == MODEM_EVENT_WIFI_STA_DISCONN) {
            ESP_LOGW(TAG, "------ Modem Board Event: 热点设备断开。");

        } else if (event_id == MODEM_EVENT_UNKNOWN) {
            ESP_LOGW(TAG, "------ Modem Board Event: 我也不知道啥错！");
        }
    }
}

/**
 * @brief 初始化函数。
 * @param
 * @return
 */
esp_err_t app_modem_init(void) {
    modem_config_t modem_config = MODEM_DEFAULT_CONFIG();
    modem_config.handler = app_modem_event_handler;
    // 不允许执行 modem_board_force_reset()，因为这个函数内部执行的低电平脉冲宽度不足 2 秒，参考 A7670C_R2_硬件设计手册_V1.06.pdf 模块复位章节。
    modem_config.flags |= MODEM_FLAGS_INIT_NOT_FORCE_RESET;
    esp_err_t board_ret = modem_board_init(&modem_config);
    return board_ret;
}