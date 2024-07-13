/**
 * @brief   SD 卡初始化，本地文件读写。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

 /**
 * @brief SD 卡引脚配置，这个板只有1条数据脚。
 */
#define SDMMC_WIDTH         1
#define SDMMC_CMD           4
#define SDMMC_CLK           5
#define SDMMC_DATA          6

 /**
 * @brief ESP-IDF 的示例代码，挂载点是小写。
 */
#define SDMMC_MOUNT_POINT   "/sdcard"

 /**
 * @brief 日志目录。
 *        必须大写！为啥啊！
 *        实际测试，传参是小写，建立的文件名还是大写！
 *        坑死我啊，目录名必须大写，文件名也必须大写，并且不能太长！
 */
#define APP_SD_LOG_DIR      SDMMC_MOUNT_POINT"/LOG"

 /**
 * @brief 日志 TAG。
 */
static const char* TAG = "app_sd";

/**
* @brief 日志文件。
*/
FILE* app_sd_log_file = NULL;

/**
* @brief 增加写日志到文件的功能，保留日志输出到 UART。
*/
static int app_sd_write_log_file(const char* fmt, va_list args) {
    int ret_uart = vprintf(fmt, args);// 先写 UART。
    int ret_file = 0;
    if (app_sd_log_file != NULL) {
        ret_file = vfprintf(app_sd_log_file, fmt, args);// 再写文件。
    }
    return ret_uart < 0 ? ret_uart : ret_file;
}

/**
* @brief 日志文件。
*/
void app_sd_create_log_file(void) {
    time_t now;
    struct tm timeinfo;
    time(&now); // 获取当前时间（秒）。
    gmtime_r(&now, &timeinfo); // 将时间转换为 UTC 时间。
    char log_file_name[64];
    if (timeinfo.tm_year < (2024 - 1900)) {// 如果是无效时间。
        snprintf(log_file_name, sizeof(log_file_name), APP_SD_LOG_DIR"/LOG.TXT");// 文件名使用常量值，随便什么名都行，能打开文件就行。
    } else {
        strftime(log_file_name, sizeof(log_file_name), APP_SD_LOG_DIR"/%y%m%d%H.TXT", &timeinfo);// 文件名使用日期时间字符串。
    }
    app_sd_log_file = fopen(log_file_name, "a");
    if (app_sd_log_file == NULL) {
        ESP_LOGI(TAG, "------ SD 卡创建日志文件：失败！文件名：%s", log_file_name);
    } else {
        ESP_LOGI(TAG, "------ SD 卡创建日志文件：完成。文件名：%s", log_file_name);
        esp_log_set_vprintf(app_sd_write_log_file);
    }
}

/**
* @brief SD 卡创建目录。
*/
static int app_sd_mkdir(const char* path) {
    if (access(path, F_OK) == -1) {// Check if the directory exists
        if (mkdir(path, 0700) == -1) {// Create the directory
            ESP_LOGI(TAG, "------ SD 卡创建目录：失败！%s: %s", path, strerror(errno));
            return -1;
        }
    }
    return 1;
}

/**
 * @brief 初始化函数。
 * @param
 * @return
 */
esp_err_t app_sd_init(void) {

    const char mount_point[] = SDMMC_MOUNT_POINT;

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    slot_config.width = SDMMC_WIDTH;
    slot_config.cmd = SDMMC_CMD;
    slot_config.clk = SDMMC_CLK;
    slot_config.d0 = SDMMC_DATA;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,    // 挂载失败格式化。
        .max_files = 10,                    // 最多打开文件数。
        .allocation_unit_size = 16 * 1024   // 格式化单元大小。
    };

    sdmmc_card_t* card;
    esp_err_t ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        return ret;
    }
    ESP_LOGI(TAG, "------ SD 卡挂载点：%s", mount_point);
    sdmmc_card_print_info(stdout, card);

    int log_dir_ret = app_sd_mkdir(APP_SD_LOG_DIR);// 创建日志目录。
    if (log_dir_ret == -1) {
        return ESP_FAIL;
    }
    return ESP_OK;
}