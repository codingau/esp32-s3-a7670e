#include <stdio.h>
#include "../nmea/parser_types.h"
#include "atcsq.h"
#include "parse.h"

int init(nmea_parser_s* parser) {
    NMEA_PARSER_TYPE(parser, AT_CSQ);
    NMEA_PARSER_PREFIX(parser, "CSQ: ");
    return 0;
}

int allocate_data(nmea_parser_s* parser) {
    parser->data = malloc(sizeof(at_csq_s));
    if (NULL == parser->data) {
        return -1;
    }
    return 0;
}

int set_default(nmea_parser_s* parser) {
    memset(parser->data, 0, sizeof(at_csq_s));
    return 0;
}

int free_data(nmea_s* data) {
    free(data);
    return 0;
}

int parse(nmea_parser_s* parser, char* value, int val_index) {
    at_csq_s* data = (at_csq_s*)parser->data;
    switch (val_index) {
        case 0:
            data->rssi = atoi(value);
            break;

        case 1:
            data->ber = atoi(value);
            break;
        default:
            break;
    }
    return 0;
}
