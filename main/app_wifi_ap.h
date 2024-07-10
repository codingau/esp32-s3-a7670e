#pragma once

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "usbh_modem_board.h"
#include "usbh_modem_wifi.h"

esp_err_t app_wifi_ap_init(char* dev_addr);