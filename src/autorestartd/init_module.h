//
// Created by sheverdin on 6/24/24.
//

#ifndef TF_AR_POE_INIT_MODULE_H
#define TF_AR_POE_INIT_MODULE_H

#include "../global_includs.h"
#include "init_module/port_manager.h"

// Основные API
void system_init(int argc, char **argv);
void init_ports(void);
void getPortsParam(void);
void rebootParamInit(void);
void log_initialized_vars(void);
void handle_reboot_params(void);

#endif //TF_AR_POE_INIT_MODULE_H
