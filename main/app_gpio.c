/**
 * @brief   GPIO 初始化，执行模块。
 *
 * @author  nyx
 * @date    2024-07-18
 */
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "driver/gpio.h"

#include "app_config.h"

 /**
 * @brief 日志 TAG。
 */
static const char* TAG = "app_gpio";

/**
 * @brief GPIO 操作函数。
 * @param level
 */
int app_gpio_set_level(gpio_num_t gpio_num, uint32_t level) {
    int cur_level = gpio_get_level(gpio_num);
    if (cur_level == level) {
        return 0;
    } else {
        gpio_set_level(gpio_num, level);
        ESP_LOGI(TAG, "------ GPIO 电平状态改变: %lu", level);
        return 1;
    }
}

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_gpio_init(void) {
    gpio_config_t gpio_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT_OUTPUT,
        .pin_bit_mask = APP_GPIO_PIN_BIT_MASK,
        .pull_down_en = 0,
        .pull_up_en = 0
    };
    esp_err_t gpio_ret = gpio_config(&gpio_conf);
    return gpio_ret;
}
