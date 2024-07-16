/**
 * @brief   LED 初始化，闪灯状态控制。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#pragma once

 /**
  * @brief 设置红灯闪烁次数。
  * @return
  */
void app_led_error_num(uint32_t num);

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_led_init(void);
