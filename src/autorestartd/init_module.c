/**
 * @file init_module.c
 * @brief Модуль инициализации системы и управления портами
 */

#include "init_module.h"
#include "port_manager.h"
#include "init_module/config_loader.h"
#include "init_module/error_handler.h"
#include <syslog.h>

void system_init(void)
{
    error_handler_init();
    port_manager_init();
    if (config_load_main() != 0)
    {
        error_register(ERR_UNAVAILABLE_RESOURCE, ERR_LEVEL_CRITICAL,
                       "Main portConfigInfo load failed", __FILE__, __LINE__);
    }
    port_manager_load_config();
    //config_init_timer();
    syslog(LOG_INFO, "System initialized successfully");
    //config_log_all_params();
    port_manager_log_all_configs();
}






