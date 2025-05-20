/**
 * @file init_module.c
 * @brief Модуль инициализации системы и управления портами
 */

#include "init_module.h"
#include "init_module/port_manager.h"
#include "init_module/config_loader.h"
#include "init_module/error_handler.h"
#include <syslog.h>

void system_init(int argc, char** argv)
{
    error_handler_init();
    port_manager_init();

    if (config_load_main() != 0)
    {
        error_register(ERR_UNAVAILABLE_RESOURCE, ERR_LEVEL_CRITICAL,
                       "Main config load failed", __FILE__, __LINE__);
    }
    port_manager_load_config();
    port_manager_init_reboot_info();
    config_init_timer();
    config_log_all_params();
    port_manager_log_all_configs();
    syslog(LOG_INFO, "System initialized successfully");
}

static TestType get_test_type(const char *str);
static bool_t get_alarm_state(const char *str);
static void reset_port_config(PortConfig *config, uint8_t isReboot);
//static void reset_port_stats(PortResetInfo *resetInfo, uint8_t isReboot);

//**********************************************
//  Инициализация параметров портов
//**********************************************
void getPortsParam(void)
{
    for (uint8_t i = 0; i < NUM_PORTS; i++)
    {
        PortConfig config;
        if (port_manager_get_config(i, &config))
        {
            error_register(ERR_CFG_LOAD, ERR_LEVEL_WARNING,
                           "Failed to get config for port %d", __FILE_NAME__, i+1);
            continue;
        }
        syslog(LOG_DEBUG, "Port %d Config: TestType=%d, Alarm=%d",
               i+1, config.testType, config.alarm);
    }
}

//**********************************************
//  Инициализация параметров перезагрузки
//**********************************************
//void rebootParamInit(void)
//{
//    for (uint8_t i = 0; i < NUM_PORTS; i++)
//    {
//        RebootInfo rebootParams =
//        {
//            .rebootDelay = 15,
//            .rebootCount = 0
//        };
//        port_manager_set_reboot_params(i, &rebootParams);
//    }
//}

//**********************************************
//  Преобразование строки в тип теста
//**********************************************

static TestType get_test_type(const char *str)
{
    const char *testTypes[] = {"disable", "link", "ping", "speed"};
    for (TestType i = TEST_DISABLE; i < TEST_MAX; i++) {
        if (strcmp(str, testTypes[i]) == 0)
            return i;
    }
    return TEST_DISABLE;
}

//**********************************************
//  Преобразование строки в состояние тревоги
//**********************************************
static bool_t get_alarm_state(const char *str)
{
    return (strcmp(str, "enable") == 0) ? BOOL_ENABLED : BOOL_DISABLED;
}

//**********************************************
//  Сброс конфигурации порта
//**********************************************
static void reset_port_config(PortConfig *config, uint8_t isReboot)
{
    config->testType = TEST_DISABLE;
    config->alarm = BOOL_DISABLED;
    if (!isReboot)
    {
        config->maxSpeed = 0;
        config->minSpeed = 0;
    }
}

//**********************************************
//  Сброс статистики порта
//**********************************************
//static void reset_port_stats(PortResetInfo *resetInfo, uint8_t isReboot) {
//    resetInfo->errorCode = ERR_OK;
//    if (!isReboot) {
//        resetInfo->totalReboots = 0;
//    }
//}

//**********************************************
//  Запуск таймера тестирования
//**********************************************
//void runTimer(void) {
//    ConfigEntry *config = config_get_all();
//    config[MAIN_TIME].value = time(NULL) + config[TEST_DELAY].value;
//}

//**********************************************
//  Вывод информации о портах (для тестирования)
//**********************************************

void log_initialized_vars(void)
{
    port_manager_log_state();
    config_loader_log_state();
}