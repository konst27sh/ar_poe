//
// Created by sheverdin on 5/16/25.
//

#ifndef TF_AUTORESTART_COMMAND_EXECUTOR_H
#define TF_AUTORESTART_COMMAND_EXECUTOR_H

/**
 * @file command_executor.h
 * @brief Интерфейс для выполнения системных команд и работы с UBUS
 */

#include <stdio.h>
#include <jansson.h>
#include "config_loader.h"
#include "port_manager.h"

#define MAX_CMD_LEN     128
#define MAX_RESPONSE    1024

// Типы команд
typedef enum {
    CMD_GET_PORT_STATUS,
    CMD_REBOOT_PORT,
    CMD_UPDATE_CONFIG
}CommandType;

/**
 * @brief Выполняет UBUS-команду и возвращает JSON-ответ
 * @param command_template Шаблон команды (с %s для аргумента)
 * @param arg Аргумент команды
 * @return Указатель на разобранный JSON-объект или NULL при ошибке
 */
json_t* execute_ubus_command(const char* command_template, const char* arg);

/**
 * @brief Обрабатывает команду управления портом
 * @param port_num Номер порта (0-based)
 * @param cmd_type Тип команды
 * @return 0 при успехе, -1 при ошибке
 */
int handle_port_command(uint8_t port_num, CommandType cmd_type);

/**
 * @brief Обновляет конфигурацию через UBUS
 * @param section_name Имя секции конфига
 */
void update_config(const char* section_name);

#endif //TF_AUTORESTART_COMMAND_EXECUTOR_H
