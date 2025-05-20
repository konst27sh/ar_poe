//
// Created by sheverdin on 5/15/25.
//

#include "error_handler.h"

#include <string.h>
#include <syslog.h>

#define MAX_ERROR_HISTORY 10

static ErrorEntry error_history[MAX_ERROR_HISTORY];
static uint8_t error_index = 0;

static const struct
{
    error_code_t code;
    const char *message;
}

error_messages[] =
{
    {ERR_OK,                    "No error"},
    {ERR_PORT_DISABLE,          "Port is disabled"},
    {ERR_UNAVAILABLE_RESOURCE,  "Invalid configuration"},
    {ERR_NULL_OBJ,              "Memory allocation failed"}
};

void error_handler_init() {
    memset(error_history, 0, sizeof(error_history));
}

void error_register(
        error_code_t code,
        ErrorLevel level,
        const char *message,
        const char *file,
        uint32_t line
                    )
    {
    ErrorEntry entry = {
            .code = code,
            .level = level,
            .timestamp = time(NULL),
            .message = message,
            .source_file = file,
            .line_number = line
    };

    // Циклический буфер
    error_history[error_index] = entry;
    error_index = (error_index + 1) % MAX_ERROR_HISTORY;

    // Логирование в syslog
    int priority;
    switch(level) {
        case ERR_LEVEL_DEBUG:    priority = LOG_DEBUG; break;
        case ERR_LEVEL_INFO:     priority = LOG_INFO; break;
        case ERR_LEVEL_WARNING:  priority = LOG_WARNING; break;
        case ERR_LEVEL_CRITICAL: priority = LOG_CRIT; break;
        default:                priority = LOG_ERR;
    }

    syslog(priority, "[%s] %s (File: %s, Line: %d)",
           error_code_to_str(code),
           message,
           file,
           line);
}

const ErrorEntry* error_get_last()
{
    if (error_index == 0) return &error_history[MAX_ERROR_HISTORY - 1];
    return &error_history[error_index - 1];
}

const char* error_code_to_str(error_code_t code)
{
    for (size_t i = 0; i < sizeof(error_messages)/sizeof(*error_messages); i++)
    {
        if (error_messages[i].code == code) {
            return error_messages[i].message;
        }
    }
    return "Unknown error";
}





