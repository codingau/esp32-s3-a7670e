/**
 * @brief   SD 卡初始化，本地文件读写。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#pragma once

 /**
 * @brief 缓存文件。
 */
extern FILE* app_sd_cache_file;

/**
 * @brief 写入缓存文件。
 */
void app_sd_write_cache_file(char* dev_time, char* json);

/**
 * @brief 推送缓存数据。
 */
void app_sd_publish_cache(int cur_ts);

/**
* @brief 确保写出日志内容到 SD 卡。
*        fsync() 执行比较消耗性能，所以由外部调用，隔一段时间执行一次。
*/
void app_sd_fsync_log_file(void);

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_sd_init(void);
