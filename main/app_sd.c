/**
 * @brief   SD 卡初始化，文件读写。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#define SDMMC_WIDTH         1
#define SDMMC_CMD           4
#define SDMMC_CLK           5
#define SDMMC_DATA          6
#define SDMMC_MOUNT_POINT   "/sdcard"

 /**
 * @brief 日志 TAG。
 */
static const char* TAG = "app_sd";

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
    ESP_LOGI(TAG, "SD 卡挂载点：%s", SDMMC_MOUNT_POINT);
    sdmmc_card_print_info(stdout, card);
    return ESP_OK;
}