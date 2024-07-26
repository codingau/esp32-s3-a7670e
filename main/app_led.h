/**
 * @brief   LED 初始化，闪灯状态控制。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#pragma once

 /**
  * @brief 循环任务，消息发送成功：绿灯，发送失败保存到 SD 卡：黄灯。
  * @param ts
  * @param green
  */
void app_led_loop_mqtt(uint32_t ts, int green);

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
