/**
 * @brief   BLE 初始化，蓝牙接近开关功能。
 *
 * @author  nyx
 * @date    2024-07-10
 */
#include <stdio.h>
#include <string.h>
#include <stdatomic.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "esp_nimble_hci.h"
#include "host/util/util.h"
#include "driver/gpio.h"

#include "app_gpio.h"
#include "app_config.h"

 /**
 * @brief 日志 TAG。
 */
static const char* TAG = "app_ble";

/**
 * @brief 蓝牙钥匙最后刷新时间。
 */
_Atomic int app_ble_disc_ts = ATOMIC_VAR_INIT(-3600000);// 提前一小时的毫秒值，不管以后怎么改参数也应该够用了。

/**
 * @brief 发现设备后的事件。
 * @param event
 * @param arg
 * @return
 */
static int app_ble_gap_event(struct ble_gap_event* event, void* arg) {
    switch (event->type) {
        case BLE_GAP_EVENT_DISC:
            // ESP_LOGI(TAG, "------ BLE Address: %02x:%02x:%02x:%02x:%02x:%02x",
            //     event->disc.addr.val[0], event->disc.addr.val[1], event->disc.addr.val[2],
            //     event->disc.addr.val[3], event->disc.addr.val[4], event->disc.addr.val[5]);
            atomic_store(&app_ble_disc_ts, esp_log_timestamp());// 更新最后扫描到的时间。
            break;
        default:
            break;
    }
    return 0;
}

/**
 * @brief 启动发现任务。
 * @param
 */
void app_ble_gap_discovery(void) {

    ble_addr_t white_list[] = APP_BLE_WHITE_LIST;
    int white_list_count = (sizeof(white_list) / sizeof(ble_addr_t));
    ble_gap_wl_set(white_list, white_list_count);// 设置白名单。

    struct ble_gap_disc_params disc_params;
    disc_params.itvl = BLE_GAP_SCAN_ITVL_MS(400);// 400 毫秒中的 100 毫秒用来扫描。
    disc_params.window = BLE_GAP_SCAN_WIN_MS(100);// 我的蓝色蓝牙钥匙，平均每 0.5 秒发射一次广播。
    disc_params.filter_policy = BLE_HCI_SCAN_FILT_USE_WL;// 使用白名单模式。
    disc_params.limited = 0;// 非有限发现模式。
    disc_params.passive = 1;// 被动扫描。
    disc_params.filter_duplicates = 0;// 不过滤重复，每次扫描到都触发 BLE_GAP_EVENT_DISC 事件。
    ble_gap_disc(BLE_OWN_ADDR_PUBLIC, BLE_HS_FOREVER, &disc_params, app_ble_gap_event, NULL);
}

/**
 * @brief 启动蓝牙。
 * @param param
 */
static void app_ble_host_task(void* param) {
    nimble_port_run();// 此函数会被阻塞，只有执行 nimble_port_stop() 时，此函数才会返回。
    // 以下的的任何代码都不会被执行。
    nimble_port_freertos_deinit();// 此行永远不会被执行。
}

/**
 * @brief 蓝牙钥匙检测 LEAVE 任务。
 * @param param
 */
static void app_ble_leave_task(void* param) {
    while (1) {
        int cur_ts = esp_log_timestamp() / 1000;// 系统启动以后的秒数。
        int ble_ts = atomic_load(&app_ble_disc_ts) / 1000;// 最后一次扫描到蓝牙开关的秒数。
        if (cur_ts - ble_ts > APP_BLE_LEAVE_TIMEOUT) {// 如果蓝牙开关离开 1 分钟，则关闭。
            int ret = app_gpio_set_level(APP_GPIO_NUM_BLE, 0);
            if (ret == 1) {
                ESP_LOGI(TAG, "------ 蓝牙接近开关: 关闭。");
            }
        } else {
            int ret = app_gpio_set_level(APP_GPIO_NUM_BLE, 1);// 如果蓝牙开关在 1 分钟内，则开启。
            if (ret == 1) {
                ESP_LOGI(TAG, "------ 蓝牙接近开关: 开启。");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1000));// 延迟 1 秒。
    }
}

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_ble_init(void) {
    esp_err_t ble_ret = nimble_port_init();
    if (ble_ret != ESP_OK) {
        return ble_ret;
    }
    ble_hs_cfg.sync_cb = app_ble_gap_discovery;
    nimble_port_freertos_init(app_ble_host_task);
    xTaskCreate(app_ble_leave_task, "app_ble_leave_task", 2048, NULL, 10, NULL);// 蓝牙钥匙检测 LEAVE 任务。
    return ESP_OK;
}