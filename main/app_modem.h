/**
 * @brief   MODEM 初始化，上网功能。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#pragma once

 /**
  * @brief MODEM 重置函数。
  * @param
  */
void app_modem_reset(void);

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_modem_init(void);
