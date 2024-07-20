// add by nyx 2024-07-19
#pragma once

typedef struct {
    nmea_s base;
    int rssi;
    int ber;
} at_csq_s;
