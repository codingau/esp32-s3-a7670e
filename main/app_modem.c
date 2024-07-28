/**
 * @brief   MODEM 初始化，上网功能。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#include <stdatomic.h>
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "usbh_modem_board.h"
#include "usbh_modem_wifi.h"

#include "app_at.h"
#include "app_gpio.h"
#include "app_config.h"

 /**
 * @brief 日志 TAG。
 */
static const char* TAG = "app_modem";

/**
 * @brief 网络是否已连接。
 */
_Atomic int app_modem_net_conn = ATOMIC_VAR_INIT(0);

/**
 * @brief MODEM 重置函数。
 * @param
 */
void app_modem_reset(void) {
    app_at_send_command("AT+CRESET\r\n");
    ESP_LOGI(TAG, "------ 使用 UART 发送 AT 命令，重置 MODEM。");
}

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
            atomic_store(&app_modem_net_conn, 1);

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
    modem_config.flags |= MODEM_FLAGS_INIT_NOT_ENTER_PPP;// 不自动进入 PPP，下边代码手动进入。
    // If auto enter ppp and block until ppp got ip。因为不自动进入 PPP，所以下边这个参数，不起任何作用。
    modem_config.flags |= MODEM_FLAGS_INIT_NOT_BLOCK;// modem_board_init() 函数，非阻塞。 

    // modem_config.
    // 修正 modem_board_force_reset() 内部代码以适应当前开发板，
    // 因为 modem_board_force_reset() 函数内部执行的低电平脉冲宽度不足 2 秒，参考 A7670C_R2_硬件设计手册_V1.06.pdf 模块复位章节。
    modem_config.reset_func = app_modem_reset;
    esp_err_t board_ret = modem_board_init(&modem_config);
    if (board_ret == ESP_OK) {
        board_ret = modem_board_ppp_start(30000);
        modem_board_ppp_auto_connect(true);
    }
    return board_ret;
}