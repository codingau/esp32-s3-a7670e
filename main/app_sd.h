/**
 * @brief   SD 卡初始化，本地文件读写。
 *
 * @author  nyx
 * @date    2024-06-28
 */
#pragma once

 /**
 * @brief 日志文件。
 */
extern FILE* app_sd_log_file;

/**
 * @brief 创建日志文件。
 */
void app_sd_create_log_file(void);

/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_sd_init(void);
