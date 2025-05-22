//
// Created by sheverdin on 5/15/25.
//

/**
 * @file config_loader.c
 * @brief Загрузка и управление конфигурацией приложения
 */

#include <stdio.h>

#include <string.h>
#include "config_loader.h"
#include "utils_lib/utils_module.h"
#include <time.h>

// Приватные константы
#define UBUS_CMD_TEMPLATE    "ubus call uci get '{\"config\":\"%s\", \"section\":\"%s\"}'"
#define UBUS_SPEED_TEMPLATE    "ubus call net_stat getStatus '{\"port\":\"lan%d\", \"param\":\"ifInOctets\"}'"
#define MAX_UBUS_RESPONSE    512
#define MAX_PARAM_NAME       32

typedef struct
{
    const char *name;
    int min_val;
    int max_val;
    int default_val;
}ConfigParamDef;

// Таблица параметров с валидацией
static const ConfigParamDef global_param_defs[] =
{
    {"test_delay", 1,  60,  3},
    {"test_num",   1,  100, 10},
    {"max_reset",  1,  20,  3},
    {"main_time",  0,  0,   0}
};

Timer global_timer;

config_type init_configOptions[MAX_PARM] =
{
    {
        .optionName = "test_delay",
        .value    = 0,
    },
    {
        .optionName = "test_num",
        .value = 0,
    },
    {
        .optionName = "max_reset",
        .value = 0,
    },
    {
        .optionName = "main_time",
        .value = 0,
    }
};

// Глобальный конфиг
static ConfigEntry global_config[MAX_PARAMS];

/**
 * @brief Парсит JSON-ответ от UBUS
 * @param json_str Строка с JSON
 * @param param_name Имя параметра
 * @param[out] value Буфер для значения
 * @return 0 при успехе, -1 при ошибке
 *
 */
int parse_json_param(json_t* values, const char* param_name, char* value)
{
    if (values == NULL || param_name == NULL || value == NULL)
    {
        syslog(LOG_ERR, "NULL pointer in parse_json_param");
        return -1;
    }
    json_t* param = json_object_get(values, param_name);
    if (!param || !json_is_string(param)) return -1;

    strncpy(value, json_string_value(param), MAX_PARAM_VALUE - 1);
    value[MAX_PARAM_VALUE - 1] = '\0';
    return 0;
}

/**
 * @brief Загружает конфигурацию секции
 * @param config_name Имя конфига (например, "tf_autorestart")
 * @param section_name Имя секции (например, "ar_demon")
 */
json_t* config_load_section(const char *config_name, const char *section_name)
{
    char cmd[128];
    char response[MAX_UBUS_RESPONSE] = {0};
    FILE *pipe = NULL;

    snprintf(cmd, sizeof(cmd), UBUS_CMD_TEMPLATE, config_name, section_name);
    pipe = popen(cmd, "r");
    if (!pipe) return NULL;

    char chunk[128];
    while (fgets(chunk, sizeof(chunk), pipe)) {
        strncat(response, chunk, sizeof(response) - strlen(response) - 1);
    }
    pclose(pipe);

    json_error_t error;
    return json_loads(response, 0, &error);
}

json_t* config_load_speed(uint8_t portnum)
{
    char cmd[128];
    char response[32] = {0};
    FILE *pipe = NULL;

    snprintf(cmd, sizeof(cmd), UBUS_SPEED_TEMPLATE, portnum);
    pipe = popen(cmd, "r");
    if (!pipe) return NULL;

    char chunk[128];
    while (fgets(chunk, sizeof(chunk), pipe)) {
        strncat(response, chunk, sizeof(response) - strlen(response) - 1);
    }
    pclose(pipe);

    json_error_t error;
    return json_loads(response, 0, &error);
}

json_t* config_get_section(const char* section_name)
{
    for (size_t i = 0; i < MAX_PARAMS; i++)
    {
        if (strcmp(section_name, init_configOptions[i].optionName) == 0)
        {
            return json_object_get(global_config[i].json_data, "values");
        }
    }
    return NULL;
}

int config_get_int_param(json_t *values, const char *param_name, int *value) {
    json_t *param = json_object_get(values, param_name);
    if (!param || !json_is_string(param)) return -1;

    *value = atoi(json_string_value(param));
    return 0;
}

int config_get_str_param(json_t *values, const char *param_name, char *buf, size_t buf_size) {
    json_t *param = json_object_get(values, param_name);
    if (!param || !json_is_string(param)) return -1;

    strncpy(buf, json_string_value(param), buf_size - 1);
    buf[buf_size - 1] = '\0';
    return 0;
}

/**
 * @brief Возвращает значение параметра конфигурации
 * @param param_name Имя параметра
 * @return Значение или -1 если не найден
 */
int config_get_value(const char *param_name)
{
    for (size_t i = 0; i < MAX_PARAMS; i++)
    {
        if (strcmp(param_name, init_configOptions[i].optionName) == 0)
        {
            return init_configOptions[i].value;
        }
    }
    return -1;
}

/**
 * @brief Returns the entire configuration
 * @param[out] entries Буфер для записи
 * @param max_entries Максимальное количество записей
 * @return Число записанных записей
 */
void config_log_all_params(void)
{
    syslog(LOG_INFO, "=== Global Configuration ===");

    for (size_t i = 0; i < MAX_PARAMS; i++)
    {
        syslog(LOG_INFO, "Parameter [%s] = %d", init_configOptions[i].optionName, init_configOptions[i].value);
    }
    //syslog(LOG_INFO, "Main timer: start = %ld, interval = %d [s]", global_timer.start_time, global_timer.interval);
}

static int parse_config_from_json(const char *json_str) {
    json_error_t error;
    json_t *root = json_loads(json_str, 0, &error);
    syslog(LOG_ERR, "json_str =: %s ", json_str);
    if (!root) {
        syslog(LOG_ERR, "JSON parse error 2: %s (line %d)", error.text, error.line);
        return -1;
    }

    for (int i = 0; i < MAX_PARM - 1; i++) {
        const char *param_name = init_configOptions[i].optionName;
        json_t *param = json_object_get(json_object_get(root, "values"), param_name);

        if (param && json_is_string(param)) {
            const char *value_str = json_string_value(param);
            init_configOptions[i].value = strtol(value_str, NULL, 10);
        }
    }

    json_decref(root);
    return 0;
}

/**
 * @brief Загружает основную конфигурацию приложения
 * @return 0 при успехе, -1 при ошибке
 */
int config_load_main(void)
{
    syslog(LOG_DEBUG, "Loading main config...");
    json_t *root = config_load_section("tf_autorestart", "ar_demon");
    if (root == NULL)
    {
        syslog(LOG_ERR, "json root not valid");
        return -1;
    }

    json_t *values = json_object_get(root, "values");
    if (values == NULL)
    {
        syslog(LOG_ERR, "values not valid");
        json_decref(root);
        return -1;
    }

    for (size_t i = 0; i < MAX_PARAMS; i++)
    {
        const ConfigParamDef *def = &global_param_defs[i];
        char value_str[MAX_PARAM_VALUE] = {0};

        if (parse_json_param(values, def->name, value_str) == 0)
        {
            init_configOptions[i].value = (int)strtol(value_str, NULL, 10);
        } else
        {
            init_configOptions[i].value = def->default_val;
            syslog(LOG_WARNING, "Using default for %s: %d", def->name, def->default_val);
        }
    }
    syslog(LOG_INFO, "Global param set successfully");
    json_decref(root);
    return 0;
}



// Новая функция для загрузки из строки
int load_config_from_string(const char *json_str)
{
    return parse_config_from_json(json_str);
}

void config_loader_log_state(void)
{
    syslog(LOG_DEBUG, "=== Config Loader State ===");
    for (size_t i = 0; i < MAX_PARAMS; i++)
    {
        syslog(LOG_DEBUG, "Parameter %s = %d",
               init_configOptions[i].optionName,
               init_configOptions[i].value);
    }
}

TestType config_get_test_type(const char* section_name) {
    json_t* section = config_get_section(section_name);
    if (!section) return TEST_DISABLE;

    char mode[MAX_PARAM_VALUE];
    if (parse_json_param(section, "mode", mode) == 0)
    {
        if (strcmp(mode, "link") == 0) return TEST_LINK;
        if (strcmp(mode, "ping") == 0) return TEST_PING;
        if (strcmp(mode, "speed") == 0) return TEST_SPEED;
    }
    return TEST_DISABLE;
}

bool_t config_get_bool(const char* section_name, const char* param)
{
    json_t* section = config_get_section(section_name);
    if (!section) return BOOL_DISABLED;

    char value[MAX_PARAM_VALUE];
    if (parse_json_param(section, param, value) == 0)
    {
        return (strcasecmp(value, "enable") == 0);
    }
    return BOOL_DISABLED;
}

void config_get_string(const char* section_name, const char* param, char* buffer, size_t buf_size)
{
    json_t* section = config_get_section(section_name);
    if (!section) {
        buffer[0] = '\0';
        return;
    }

    char value[MAX_PARAM_VALUE];
    if (parse_json_param(section, param, value) == 0) {
        strncpy(buffer, value, buf_size - 1);
        buffer[buf_size - 1] = '\0';
    } else {
        buffer[0] = '\0';
    }
}

void config_init_timer(void)
{
    //global_timer.start_time = time(NULL);
    //init_configOptions[MAIN_TIME].value = (int) time(NULL) + init_configOptions[TEST_DELAY].value;
    //syslog(LOG_ERR, "start_time =  %dd --- time now = %ld", init_configOptions[MAIN_TIME].value, time(NULL));
}

