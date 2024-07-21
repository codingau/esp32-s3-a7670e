/**
 * @brief   JSON 对象转换。
 *
 * @author  nyx
 * @date    2024-07-12
 */
#include <stdio.h>
#include <string.h>
#include "cJSON.h"

#include "app_main.h"

char* app_json_serialize(const app_main_data_t* data) {

    cJSON* root = cJSON_CreateObject();

    cJSON_AddStringToObject(root, "dev_addr", data->dev_addr);
    cJSON_AddStringToObject(root, "dev_time", data->dev_time);
    cJSON_AddNumberToObject(root, "log_ts", data->log_ts);
    cJSON_AddNumberToObject(root, "ble_ts", data->ble_ts);
    cJSON_AddStringToObject(root, "gnss_time", data->gnss_time);
    cJSON_AddBoolToObject(root, "gnss_valid", data->gnss_valid);
    cJSON_AddNumberToObject(root, "sat", data->sat);
    cJSON_AddNumberToObject(root, "alt", data->alt);
    cJSON_AddNumberToObject(root, "lat", data->lat);
    cJSON_AddNumberToObject(root, "lon", data->lon);
    cJSON_AddNumberToObject(root, "spd", data->spd);
    cJSON_AddNumberToObject(root, "trk", data->trk);
    cJSON_AddNumberToObject(root, "mag", data->mag);
    cJSON_AddNumberToObject(root, "f", data->f);

    char* json_str = cJSON_PrintUnformatted(root);// 不按字段换行。
    cJSON_Delete(root);
    // cJSON_free(json_str);// 不要在这里释放！
    return json_str;
}