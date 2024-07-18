/**
 * @brief   SD 卡初始化，本地文件读写。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "app_mqtt.h"

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
 * @brief 缓存目录。
 */
#define APP_SD_CACHE_DIR      SDMMC_MOUNT_POINT"/CACHE"

 /**
 * @brief 日志 TAG。
 */
static const char* TAG = "app_sd";

/**
* @brief 日志文件。
*/
static FILE* app_sd_log_file = NULL;

/**
* @brief 日志文件写入行数。
*/
static int app_sd_log_file_write_line = 0;

/**
* @brief 缓存文件。
*/
FILE* app_sd_cache_file = NULL;

/**
* @brief 当前写入缓存的文件名，使用的时候替换数字部分，保留扩展名。
*/
static char app_sd_cur_cache_file_name[] = "12345678.TXT";

/**
* @brief 缓存目录状态。
*/
static bool app_sd_cache_dir_status = false;

/**
* @brief 当前缓存文件推送行数。
*/
static int app_sd_cur_publish_line = 0;

/**
* @brief 最近一次推送缓存的时间戳。
*/
static int app_sd_last_publish_ts = 0;

/**
* @brief 创建缓存文件。
*/
static void app_sd_create_cache_file() {
    char cache_file_path_and_name[64];
    snprintf(cache_file_path_and_name, sizeof(cache_file_path_and_name), APP_SD_CACHE_DIR"/%s", app_sd_cur_cache_file_name);
    FILE* cur_cache_file = fopen(cache_file_path_and_name, "a");
    if (cur_cache_file == NULL) {
        ESP_LOGI(TAG, "------ SD 卡创建缓存文件：失败！文件名：%s", cache_file_path_and_name);
    } else {
        if (app_sd_cache_file != NULL) {
            fclose(app_sd_cache_file);// 关闭原来的文件。
        }
        app_sd_cache_file = cur_cache_file;// 保存打开文件指针。
        ESP_LOGI(TAG, "------ SD 卡创建缓存文件：完成。文件名：%s", cache_file_path_and_name);
    }
}

/**
* @brief 输出数据到缓存文件。
*/
void app_sd_write_cache_file(char* dev_time, char* json) {
    if (!app_sd_cache_dir_status) {
        ESP_LOGI(TAG, "------ SD 卡写入缓存文件：失败！缓存目录不存在，设备时间：%s", dev_time);
        return;
    }
    if (strncmp(app_sd_cur_cache_file_name, dev_time, 8) != 0) {// 比较日期，如果不同，创建缓存文件。
        strncpy(app_sd_cur_cache_file_name, dev_time, 8);// 新文件名，日期字符串。
        app_sd_create_cache_file();
    }
    if (app_sd_cache_file == NULL) {
        return;
    }
    fprintf(app_sd_cache_file, "%s\n", json);
    fflush(app_sd_cache_file);
    fsync(fileno(app_sd_cache_file));
    ESP_LOGI(TAG, "------ SD 卡写入缓存文件，完成。文件名：%s，写入字节数：%d", app_sd_cur_cache_file_name, strlen(json));
}

/**
 * @brief 逐行读取缓存文件，并且推送。
 * @param file_name
 */
static void app_sd_publish_cache_file(char* file_name) {
    FILE* file = fopen(file_name, "r");
    if (file == NULL) {
        ESP_LOGI(TAG, "------ SD 卡推送缓存文件：开始。文件名：%s", file_name);
        return;
    }
    ESP_LOGI(TAG, "------ SD 卡推送缓存文件：开始。文件名：%s，跳过行数：%d", file_name, app_sd_cur_publish_line);
    char line[1024];
    for (int i = 0; i < app_sd_cur_publish_line; ++i) {
        fgets(line, sizeof(line), file);// 跳过 N 行。
    }

    while (fgets(line, sizeof(line), file) != NULL) {// 逐行读取文件内容。
        size_t len = strlen(line);// 去除行尾的换行符。
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0'; // 将换行符替换为 NULL 终止符。
        }
        int pub_ret = app_mqtt_publish(line);
        if (pub_ret < 0) {// 只要有一次发送失败，就跳出循环，不再继续执行。
            ESP_LOGI(TAG, "------ SD 卡推送缓存文件：中断。稍后自动重试。文件名：%s，推送行数：%d", file_name, app_sd_cur_publish_line);
            return;
        } else {
            app_sd_cur_publish_line++;// 记录已发条数。
        }
    }
    ESP_LOGI(TAG, "------ SD 卡推送缓存文件：完成。文件名：%s，推送行数：%d", file_name, app_sd_cur_publish_line);
    app_sd_cur_publish_line = 0;// 这个文件全部发送完成后，读取行数置 0，等待下一个文件。
    fclose(file);// 关闭文件。
    remove(file_name);// 删除缓存文件。
}

/**
* @brief 文件名排序，正序。
*/
int compare_file_name(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

/**
* @brief 推送缓存数据。
*/
void app_sd_publish_cache(int cur_ts) {
    if (cur_ts - app_sd_last_publish_ts < 120) {// 间隔 2 分钟检查一次。
        return;
    }
    app_sd_last_publish_ts = cur_ts;
    DIR* cache_dir = opendir(APP_SD_CACHE_DIR);
    if (cache_dir == NULL) {
        ESP_LOGI(TAG, "------ SD 卡打开缓存目录：失败！目录：%s", APP_SD_CACHE_DIR);
        return;
    }
    char** fileNameList = NULL;
    size_t fileCount = 0;
    struct dirent* entry;
    while ((entry = readdir(cache_dir)) != NULL) {// 遍历目录。
        if (entry->d_type == DT_REG) { // 只处理普通文件。
            const char* fileName = entry->d_name;
            if (strstr(fileName, ".TXT") != NULL) {// 检查文件扩展名。
                fileNameList = realloc(fileNameList, sizeof(char*) * (fileCount + 1));// 动态分配内存并存储文件名。
                fileNameList[fileCount] = strdup(fileName);
                fileCount++;
            }
        }
    }
    closedir(cache_dir);

    if (fileCount > 0) {
        ESP_LOGI(TAG, "------ SD 卡遍历缓存文件。数量：%d", fileCount);
        qsort(fileNameList, fileCount, sizeof(char*), compare_file_name);// 文件名排序，正序。
        char first_file_name[64];
        snprintf(first_file_name, sizeof(first_file_name), APP_SD_CACHE_DIR"/%s", fileNameList[0]);// 包括路径名。
        free(fileNameList[0]);
        for (size_t i = 1; i < fileCount; i++) {
            free(fileNameList[i]); // 释放每个文件名的内存。
        }
        free(fileNameList); // 释放文件名列表的内存。

        app_sd_publish_cache_file(first_file_name);// 每一次循环只推送一个文件。
    }
}

/**
* @brief 确保写出日志内容到 SD 卡。
*        fsync() 执行比较消耗性能，所以由外部调用，隔一段时间执行一次。
*/
void app_sd_fsync_log_file(void) {
    if (app_sd_log_file != NULL && app_sd_log_file_write_line > 0) {
        fsync(fileno(app_sd_log_file));
    }
}

/**
* @brief 增加写日志到文件的功能，保留日志输出到 UART。
*/
static int app_sd_write_log_file(const char* fmt, va_list args) {
    int ret_uart = vprintf(fmt, args);// 先写 UART。
    int ret_file = 0;
    if (app_sd_log_file != NULL) {
        ret_file = vfprintf(app_sd_log_file, fmt, args);// 再写文件。
        fflush(app_sd_log_file);
        app_sd_log_file_write_line++;
    }
    return ret_uart < 0 ? ret_uart : ret_file;
}

/**
* @brief 创建日志文件。
*/
static void app_sd_create_log_file(void) {
    time_t now;
    time(&now); // 获取当前时间（秒）。
    struct tm timeinfo;
    gmtime_r(&now, &timeinfo); // 将时间转换为 UTC 时间。
    char log_file_path_and_name[64];
    if (timeinfo.tm_year < (2024 - 1900)) {// 如果是无效时间。
        snprintf(log_file_path_and_name, sizeof(log_file_path_and_name), APP_SD_LOG_DIR"/LOG.TXT");// 文件名使用常量值，随便什么名都行，能打开文件就行。
    } else {
        strftime(log_file_path_and_name, sizeof(log_file_path_and_name), APP_SD_LOG_DIR"/%m%d%H%M.TXT", &timeinfo);// 文件名使用日期时间字符串。
    }
    app_sd_log_file = fopen(log_file_path_and_name, "a");
    if (app_sd_log_file == NULL) {
        ESP_LOGI(TAG, "------ SD 卡创建日志文件：失败！文件名：%s", log_file_path_and_name);
    } else {
        ESP_LOGI(TAG, "------ SD 卡创建日志文件：完成。文件名：%s", log_file_path_and_name);
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
    app_sd_create_log_file();

    int cache_dir_ret = app_sd_mkdir(APP_SD_CACHE_DIR);// 创建缓存目录。
    if (cache_dir_ret == -1) {
        return ESP_FAIL;// 创建 DATA 目录失败，数据无法写入，返回 ESP_FAIL。
    }
    app_sd_cache_dir_status = true;
    return ESP_OK;
}