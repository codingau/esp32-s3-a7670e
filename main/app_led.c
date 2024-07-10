/**
 * @brief   LED 闪灯状态控制。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_strip.h"

 /**
 * @brief
 */
#define APP_LED_GPIO 38

 /**
 * @brief 日志 TAG。
 */
static const char* TAG = "app_led";

/**
 * @brief 没啥注释的，我解释不清了。
 */
static led_strip_handle_t led_strip;

/**
 * @brief 初始化 led 状态值，启动时显示黄色。
 */
static uint32_t app_led_status_array[10][3] = {
    {1, 1, 0},
    {1, 1, 0},
    {1, 1, 0},
    {1, 1, 0},
    {1, 1, 0},
    {1, 1, 0},
    {1, 1, 0},
    {1, 1, 0},
    {1, 1, 0},
    {1, 1, 0},
};

/**
 * @brief 清除状态灯。
 */
 // static void app_led_status_clear() {
 //     for (int i = 0; i < 10; i++) {
 //         for (int j = 0; j < 3; j++) {
 //             app_led_status_array[i][j] = 0;
 //         }
 //     }
 // }

 /**
  * @brief 显示几次红灯。
  * @param num
  */
void app_led_error_num(uint32_t num) {
    for (size_t i = 0; i < num; i++) {
        app_led_status_array[i][1] = 5;
    }
    ESP_LOGI(TAG, "LED 状态改变：红色 %lu 次", num);
}

/**
 * @brief led 显示任务，10次状态灯，1次蓝色灯。
 */
static void app_led_task(void* pvParameters) {
    while (1) {
        for (int i = 0; i < 10; i++) {
            ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_set_pixel(led_strip, 0, app_led_status_array[i][0], app_led_status_array[i][1], app_led_status_array[i][2]));
            ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_refresh(led_strip));
            vTaskDelay(pdMS_TO_TICKS(500));
            ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_clear(led_strip));
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_set_pixel(led_strip, 0, 0, 0, 2));// 蓝色做为间隔符号。
        ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_refresh(led_strip));
        vTaskDelay(pdMS_TO_TICKS(500));
        ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_clear(led_strip));
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

/**
 * @brief 初始化函数。
 * @param
 */
esp_err_t app_led_init(void) {

    led_strip_config_t strip_config = {
        .strip_gpio_num = APP_LED_GPIO,               // The GPIO that connected to the LED strip's data line
        .max_leds = 1,                            // The number of LEDs in the strip,
        .led_pixel_format = LED_PIXEL_FORMAT_GRB, // Pixel format of your LED strip
        .led_model = LED_MODEL_WS2812,            // LED strip model
        .flags.invert_out = false,                // whether to invert the output signal
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,    // different clock source can lead to
        .resolution_hz = 10 * 1000 * 1000, // RMT counter clock frequency
        .flags.with_dma = true,            // DMA feature is available on ESP target like ESP32-S3
    };

    esp_err_t ret = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);
    if (ret == ESP_OK) {
        xTaskCreate(app_led_task, "app_led_task", 1024, NULL, 9, NULL);// 启动 led 显示任务。
    }
    return ret;
}
