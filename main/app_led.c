/**
 * @brief   LED 初始化，闪灯状态控制。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#include <stdatomic.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "led_strip.h"
#include "usbh_modem_board.h"

#include "app_at.h"
#include "app_gpio.h"
#include "app_gnss.h"
#include "app_ping.h"
#include "app_config.h"

 /**
 * @brief
 */
#define APP_LED_GPIO 38

 /**
 * @brief 日志 TAG。
 */
 // static const char* TAG = "app_led";

 /**
  * @brief 灯条句柄。
  */
static led_strip_handle_t led_strip;

/**
 * @brief 颜色值。
 */
static _Atomic uint32_t app_led_red = ATOMIC_VAR_INIT(0);

/**
 * @brief 颜色值。
 */
static _Atomic uint32_t app_led_green = ATOMIC_VAR_INIT(0);

/**
 * @brief 颜色值。
 */
static _Atomic uint32_t app_led_blue = ATOMIC_VAR_INIT(0);

/**
 * @brief 颜色值。
 */
static _Atomic uint32_t app_led_red2 = ATOMIC_VAR_INIT(0);

/**
 * @brief 颜色值。
 */
static _Atomic uint32_t app_led_green2 = ATOMIC_VAR_INIT(0);

/**
 * @brief 颜色值。
 */
static _Atomic uint32_t app_led_blue2 = ATOMIC_VAR_INIT(0);

/**
 * @brief led 显示任务。
 * @param param
 */
static void app_led_task(void* param) {
    while (led_strip != NULL) {
        int r = atomic_load(&app_led_red);
        int g = atomic_load(&app_led_green);
        int b = atomic_load(&app_led_blue);
        int r2 = atomic_load(&app_led_red2);
        int g2 = atomic_load(&app_led_green2);
        int b2 = atomic_load(&app_led_blue2);

        ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_set_pixel(led_strip, 0, g, r, b));// 交换 r 和 g。
        ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_refresh(led_strip));
        vTaskDelay(pdMS_TO_TICKS(200));
        ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_clear(led_strip));
        vTaskDelay(pdMS_TO_TICKS(100));
        ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_set_pixel(led_strip, 0, g2, r2, b2));// 交换 r 和 g。
        ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_refresh(led_strip));
        vTaskDelay(pdMS_TO_TICKS(200));
        ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_clear(led_strip));
        vTaskDelay(pdMS_TO_TICKS(100));
        ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_set_pixel(led_strip, 0, 0, 0, 1));// 蓝色间隔。
        ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_refresh(led_strip));
        vTaskDelay(pdMS_TO_TICKS(200));
        ESP_ERROR_CHECK_WITHOUT_ABORT(led_strip_clear(led_strip));
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

/**
 * @brief 设置 LED RGB 的值。
 * @param red
 * @param green
 * @param blue
 * @param red2
 * @param green2
 * @param blue2
 */
void app_led_set_value(uint32_t red, uint32_t green, uint32_t blue, uint32_t red2, uint32_t green2, uint32_t blue2) {
    atomic_store(&app_led_red, red);
    atomic_store(&app_led_green, green);
    atomic_store(&app_led_blue, blue);
    atomic_store(&app_led_red2, red2);
    atomic_store(&app_led_green2, green2);
    atomic_store(&app_led_blue2, blue2);
}

/**
 * @brief 初始化函数。
 * @param
 */
esp_err_t app_led_init(void) {

    led_strip_config_t strip_config = {
        .strip_gpio_num = APP_LED_GPIO,           // The GPIO that connected to the LED strip's data line
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
        xTaskCreate(app_led_task, "app_led_task", 2048, NULL, 10, NULL);// 启动 led 显示任务。
    }
    app_led_set_value(2, 1, 0, 2, 1, 0);// 启动的时候，连续闪烁黄灯。
    return ret;
}