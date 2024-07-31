/**
 * @brief   JSON 对象转换。
 *
 * @author  nyx
 * @date    2024-07-12
 */
#include <stdio.h>
#include <string.h>

#include "app_main.h"

void app_json_serialize(char* buffer, size_t buffer_size, const app_main_data_t* data) {

    char fmt[] = "{\"devAddr\":\"%s\",\"devTime\":\"%s\",\"logTs\":%d,\"bleTs\":%d,\"gpios\":\"%s\",\"gnssTime\":\"%s\",\"gnssValid\":%d,\"sat\":%d,\"alt\":%f,\"lat\":%f,\"lon\":%f,\"spd\":%f,\"trk\":%f,\"mag\":%f,\"f\":0}";

    snprintf(buffer, buffer_size, fmt,
        data->dev_addr,
        data->dev_time,
        data->log_ts,
        data->ble_ts,
        data->gpios,
        data->gnss_time,
        data->gnss_valid,
        data->sat,
        data->alt,
        data->lat,
        data->lon,
        data->spd,
        data->trk,
        data->mag
    );
}