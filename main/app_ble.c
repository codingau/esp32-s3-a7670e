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
 * @brief 蓝牙开关控制的针脚，是否输出高电平。
 *          冗余设计，两个针脚同时开关。接线的时候，随便接一条就行了。
 * @param level
 */
void app_ble_gpio_set_level(uint32_t level) {
    int cur_level = gpio_get_level(APP_BLE_GPIO_OUT_13);
    if (cur_level != level) {
        gpio_set_level(APP_BLE_GPIO_OUT_13, level);
        ESP_LOGI(TAG, "------ BLE GPIO 电平状态改变: %lu", level);
    }
    cur_level = gpio_get_level(APP_BLE_GPIO_OUT_14);
    if (cur_level != level) {
        gpio_set_level(APP_BLE_GPIO_OUT_14, level);
    }
}

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
    disc_params.itvl = BLE_GAP_SCAN_ITVL_MS(1000);// 每隔 1 秒扫描 0.5 秒。
    disc_params.window = BLE_GAP_SCAN_WIN_MS(500);// 我的蓝色蓝牙钥匙，平均每 0.5 秒发射一次广播。
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

    // 初始化 GPIO。
    gpio_config_t gpio_conf = {};
    gpio_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_conf.mode = GPIO_MODE_INPUT_OUTPUT;
    gpio_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    gpio_conf.pull_down_en = 0;
    gpio_conf.pull_up_en = 0;
    esp_err_t gpio_ret = gpio_config(&gpio_conf);
    return gpio_ret;
}