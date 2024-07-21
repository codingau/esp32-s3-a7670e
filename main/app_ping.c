/**
 * @brief   PING 初始化，网络状态检测。
 *
 * @author  nyx
 * @date    2024-07-17
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>
#include <string.h>
#include <pthread.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "ping/ping_sock.h"

#include "app_at.h"
#include "app_gnss.h"

 /**
  * @brief 日志 TAG。
  */
static const char* TAG = "app_ping";

/**
 * @brief PING 超时，时间戳。
 */
_Atomic int app_ping_timeout_ts = ATOMIC_VAR_INIT(0);

/**
 * @brief PING 正常。
 */
static void app_ping_cb_success(esp_ping_handle_t hdl, void* args) {

    atomic_store(&app_ping_timeout_ts, 0);

    uint8_t ttl;
    uint16_t seqno;
    uint32_t elapsed_time, recv_len;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
    esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
    ESP_LOGI(TAG, "------ %" PRIu32 " bytes from %s icmp_seq=%u ttl=%u time=%" PRIu32 " ms\n", recv_len, ipaddr_ntoa(&target_addr), seqno, ttl, elapsed_time);
}

/**
 * @brief PING 超时。
 */
static void app_ping_cb_timeout(esp_ping_handle_t hdl, void* args) {

    atomic_store(&app_ping_timeout_ts, esp_log_timestamp());

    uint16_t seqno;
    ip_addr_t target_addr;
    esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
    esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
    ESP_LOGW(TAG, "------ From %s icmp_seq=%u timeout\n", ipaddr_ntoa(&target_addr), seqno);
}

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_ping_init(void) {
    char ip_str[] = "8.8.8.8";
    ip_addr_t ip_addr;
    ipaddr_aton(ip_str, &ip_addr);

    esp_ping_config_t esp_ping_config = {
        .count = 0,// 无限次数。
        .interval_ms = 120000,// 2 分钟。
        .timeout_ms = 1000,
        .data_size = 64,
        .tos = 0,
        .ttl = IP_DEFAULT_TTL,
        .target_addr = ip_addr,
        .task_stack_size = ESP_TASK_PING_STACK,
        .task_prio = 11,// 任务级别，不急。
        .interface = 0,
    };

    esp_ping_callbacks_t esp_ping_callbacks = {
        .on_ping_success = app_ping_cb_success,
        .on_ping_timeout = app_ping_cb_timeout,
        .on_ping_end = NULL,
        .cb_args = NULL,
    };

    esp_ping_handle_t ping;
    esp_ping_new_session(&esp_ping_config, &esp_ping_callbacks, &ping);
    esp_err_t ret = esp_ping_start(ping);
    return ret;
}