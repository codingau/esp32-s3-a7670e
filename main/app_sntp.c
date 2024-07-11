/**
 * @brief   SNTP 初始化，同步系统时间。
 *
 * @author  nyx
 * @date    2024-07-08
 */
#include "esp_log.h"
#include "esp_netif_sntp.h"
#include "esp_sntp.h"

 /**
 * @brief NTP 服务器地址，不需要更新。经过测试，拔号成功后，从 DHCP 获到的地址也是这一个。
 */
#define APP_SNTP_DEFAULT_SERVER "pool.ntp.org"

 /**
 * @brief 日志 TAG。
 */
static const char* TAG = "app_sntp";

/**
 * @brief 时间更新回调函数。
 * @param tv
 */
static void app_stnp_sync_cb(struct timeval* tv) {

    struct tm timeinfo;
    localtime_r(&tv->tv_sec, &timeinfo);// 将 timeval 转换为 tm。
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &timeinfo);// 格式化日期和时间。

    int millis = tv->tv_usec / 1000;// 获取毫秒部分。
    ESP_LOGI(TAG, "------ SNTP 同步事件，当前时间：%s.%03d", buffer, millis);
}

/**
 * @brief 初始化函数。
 * @param
 * @return
 */
esp_err_t app_sntp_init(void) {
    esp_sntp_config_t sntp_config = ESP_NETIF_SNTP_DEFAULT_CONFIG(APP_SNTP_DEFAULT_SERVER);// 全部使用默认值。
    sntp_config.sync_cb = app_stnp_sync_cb;
    esp_err_t sntp_ret = esp_netif_sntp_init(&sntp_config);
    return sntp_ret;
}