/**
 * @brief   守护线程。
 *
 * @author  nyx
 * @date    2024-07-27
 */
#pragma once

 /**
  * @brief APP 当前状态。0 = 启动期间，1 = 运行期间。
  */
extern int app_status;


/**
 * @brief 初始化函数。
 * @return
 */
esp_err_t app_deamon_init(void);