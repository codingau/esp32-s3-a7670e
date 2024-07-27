/**
 * @brief   LED 初始化，闪灯状态控制。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#pragma once

 /**
  * @brief 设置 LED RGB 的值。
  * @param red
  * @param green
  * @param blue
  * @param red2
  * @param green2
  * @param blue2
  */
void app_led_set_value(uint32_t red, uint32_t green, uint32_t blue, uint32_t red2, uint32_t green2, uint32_t blue2);

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_led_init(void);
