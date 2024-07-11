/**
 * @brief   WIFI AP 初始化，给其它外围设备供网。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "usbh_modem_board.h"
#include "usbh_modem_wifi.h"

#include "app_wifi_ap.h"

 /**
 * @brief 日志 TAG。
 */
static const char* TAG = "app_ap";

/**
 * @brief 初始化函数。
 * @param
 * @return
 */
esp_err_t app_wifi_ap_init(char* dev_addr) {

    uint8_t mac_addr_t[6] = { 0 };
    esp_read_mac(mac_addr_t, ESP_MAC_WIFI_SOFTAP);// 热点的 MAC 地址，当芯片的硬件 ID 使用。
    sprintf(dev_addr, "%02X:%02X:%02X:%02X:%02X:%02X",
        mac_addr_t[0], mac_addr_t[1], mac_addr_t[2], mac_addr_t[3], mac_addr_t[4], mac_addr_t[5]);
    ESP_LOGI(TAG, "------ 无线热点的 MAC 地址：%s", dev_addr);

    esp_netif_t* ap_netif = modem_wifi_ap_init();
    if (ap_netif == NULL) {
        return ESP_FAIL;
    }
    modem_wifi_config_t modem_wifi_config = MODEM_WIFI_DEFAULT_CONFIG();
    return modem_wifi_set(&modem_wifi_config);
}