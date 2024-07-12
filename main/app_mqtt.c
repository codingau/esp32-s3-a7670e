/**
 * @brief   MQTT 初始化，上传数据功能。
 *
 * @author  nyx
 * @date    2024-07-08
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mqtt_client.h"

#include "app_wifi_ap.h"
#include "app_config.h"

 /**
  * @brief 日志 TAG。
  */
static const char* TAG = "app_mqtt";

/**
 * @brief MQTT 客户端。
 */
static esp_mqtt_client_handle_t mqtt5_client;

/**
 * @brief MQTT 发消息给服务器。
 * @param msg
 * @return message_id of the publish message (for QoS 0 message_id will always
 *          be zero) on success. -1 on failure, -2 in case of full outbox.
 */
int app_mqtt_publish(char* msg) {
    return esp_mqtt_client_publish(mqtt5_client, APP_MQTT_PUBLISH_TOPIC, msg, strlen(msg), APP_MQTT_QOS, 0);
}

/**
 * @brief MQTT 事件回调函数。
 * @param handler_args
 * @param base
 * @param event_id
 * @param event_data
 * @return
 */
static void app_mqtt_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "------ MQTT 事件：已连接。");
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "------ MQTT 事件：断开连接！");
            break;
        case MQTT_EVENT_PUBLISHED:
            // ESP_LOGI(TAG, "------ MQTT 事件：发布完成！");
            break;
        default:
            ESP_LOGI(TAG, "------ MQTT 其它事件。EVENT ID：%ld", event_id);
            break;
    }
}

/**
 * @brief 初始化函数。
 * @param
 * @return
 */
esp_err_t app_mqtt_init(void) {

    char will_msg[] = APP_MQTT_WILL_MSG;

    esp_mqtt_client_config_t mqtt5_cfg = {
        .broker.address.uri = APP_MQTT_URI,
        .credentials.username = APP_MQTT_USERNAME,
        .credentials.authentication.password = APP_MQTT_PASSWORD,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .network.disable_auto_reconnect = false,    // 自动连接！
        .session.last_will.topic = APP_MQTT_WILL_TOPIC,
        .session.last_will.msg = will_msg,
        .session.last_will.msg_len = strlen(will_msg),
        .session.last_will.qos = APP_MQTT_QOS,
        .session.last_will.retain = true,
    };

    mqtt5_client = esp_mqtt_client_init(&mqtt5_cfg);
    esp_mqtt_client_register_event(mqtt5_client, ESP_EVENT_ANY_ID, app_mqtt_event_handler, NULL);
    esp_err_t mqtt_ret = esp_mqtt_client_start(mqtt5_client);
    return mqtt_ret;
}