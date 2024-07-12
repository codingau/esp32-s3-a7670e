/**
 * @brief   MQTT 初始化，上传数据功能。
 *
 * @author  nyx
 * @date    2024-07-08
 */
#pragma once

 /**
  * @brief MQTT 发消息给服务器。
  * @param msg
  * @return message_id of the publish message (for QoS 0 message_id will always
  *          be zero) on success. -1 on failure, -2 in case of full outbox.
  */
int app_mqtt_publish(char* msg);

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_mqtt_init(void);
