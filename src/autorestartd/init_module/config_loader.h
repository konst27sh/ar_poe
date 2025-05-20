
/**
* @file config_loader.h
* @brief Interface for working with the configuration
* @author sheverdin
* @date 5/15/25
*/

#ifndef TF_AUTORESTART_CONFIG_LOADER_H
#define TF_AUTORESTART_CONFIG_LOADER_H

#include <stdlib.h>
#include "../../global_includs.h"
#include <jansson.h>

#define MAX_PARAMS 4

typedef enum {
    CONFIG_SECTION_DEMON,
    CONFIG_SECTION_PORT
} ConfigSectionType;

typedef struct
{
    const char *name;
    int value;
    json_t *json_data;
}ConfigEntry;

typedef struct {
    char    *optionName;
    uint8_t value;
}config_type;

typedef struct {
    time_t start_time;
    int interval;
} Timer;

extern Timer global_timer;

int config_load_main(void);
void config_load_section(const char *config_name, const char *section_name, ConfigSectionType sect_type);
int config_get_value(const char *param_name);
void config_loader_log_state(void);
json_t* config_get_section(const char* section_name);

void config_log_all_params(void);
TestType config_get_test_type(const char* section_name);
bool_t config_get_bool(const char* section_name, const char* param);
void config_get_string(const char* section_name, const char* param, char* buffer, size_t buf_size);

int load_config_from_string(const char *json_str);
void config_init_timer(void);


#endif //TF_AUTORESTART_CONFIG_LOADER_H
