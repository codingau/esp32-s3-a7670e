/**
 * @brief   JSON 对象转换。
 *
 * @author  nyx
 * @date    2024-07-12
 */
#pragma once

#include "app_main.h"

char* app_json_serialize(char* buffer, size_t buffer_size, const app_main_data_t* data);