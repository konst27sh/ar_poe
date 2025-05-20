//
// Created by sheverdin on 5/16/25.
//

#ifndef TF_AUTORESTART_ERROR_HANDLER_H
#define TF_AUTORESTART_ERROR_HANDLER_H

/**
 * @file error_handler.h
 * @brief Централизованная обработка и логирование ошибок
 */

#include <time.h>
#include <stdint.h>
#include "../../global_includs.h"

/**
 * @brief Инициализирует обработчик ошибок
 */
void error_handler_init();

/**
 * @brief Регистрирует новую ошибку
 * @param code Код ошибки
 * @param level Уровень серьёзности
 * @param message Форматированное сообщение
 * @param file Исходный файл
 * @param line Номер строки
 */

void error_register(
        error_code_t code,
        ErrorLevel level,
        const char *message,
        const char *file,
        uint32_t line
);

/**
 * @brief Возвращает последнюю зарегистрированную ошибку
 */
const ErrorEntry* error_get_last();

/**
 * @brief Преобразует код ошибки в строку
 */
const char* error_code_to_str(error_code_t code);

#endif //TF_AUTORESTART_ERROR_HANDLER_H
