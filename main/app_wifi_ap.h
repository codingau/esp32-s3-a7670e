/**
 * @brief   WIFI AP 初始化，给其它外围设备供网。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#pragma once

 /**
  * @brief 初始化函数。
  * @return
  */
esp_err_t app_wifi_ap_init(char* dev_addr);
