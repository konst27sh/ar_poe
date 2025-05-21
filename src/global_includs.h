//
// Created by sheverdin on 6/24/24.
//

#ifndef TF_AR_POE_GLOBAL_INCLUDE_H
#define TF_AR_POE_GLOBAL_INCLUDE_H

#include <syslog.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include "time.h"

#define VERSION             ("1.0.0")
#define NUM_PORTS           (8)
#define OUTPUT_MAX_LENGTH   (64)
#define WORD_LENGTH         (10)

#define MIN_SPEED           (10)
#define MAX_SPEED           (1000000)

#define MAX_PARAM_VALUE      16


typedef enum
{
    BOOL_DISABLED  = 0,
    BOOL_ENABLED   = 1,
}bool_t;

typedef enum
{
    TEST_DELAY      = 0,
    TEST_NUM        = 1,
    MAX_RESET       = 2,
    MAIN_TIME       = 3,
    MAX_PARM        = 4
}DEMON_PARAM_e;

typedef enum {
    TEST_DISABLE,
    TEST_LINK,
    TEST_PING,
    TEST_SPEED,
    TEST_MAX
}TestType;

typedef enum
{
    ERR_PORT_DISABLE            = 0x000001,
    ERR_TEST_TYPE               = 0x000002,
    ERR_TEST_LINK               = 0x000004,
    ERR_TEST_PING               = 0x000008,
    ERR_TEST_SPEED              = 0x000010,
    ERR_TIME_ALARM              = 0x000020,
    ERR_TEST_DISABLE            = 0x000040,
    ERR_POE_DOWN                = 0x000080,
    ERR_UNAVAILABLE_RESOURCE    = 0x000100,
    ERR_NULL_OBJ                = 0x000200,
    ERR_CREATE_THREAD           = 0x000400,
    ERR_JOIN_THREAD             = 0x000800,
    ERR_IP_NOT_VALID            = 0x001000,
    ERR_PoE_DISABLE             = 0x002000,
    ERR_TIME_NOT_VALID          = 0x004000,
    ERR_SPEED_VALUE             = 0x008000,
    ERR_OK                      = 0x010000,
    ERR_PORT_SHUTDOWN           = 0x020000,
    ERR_MANUAL_REBOOT           = 0x040000,
    ERR_REBOOTING               = 0x080000,
    ERR_SOCKET                  = 0x100000,
    ERR_INVALID_PORT            = 0x200000,
    ERR_HW_ACCESS               = 0x400000,
    ERR_CFG_LOAD                = 0x800000,
}error_code_t;

typedef enum
{
    CTRL_ERROR_OK       = 0x01,
    CTRL_ERROR_DEBUG    = 0x02,
    CTRL_ERROR_INACTIVE = 0x04,
    CTRL_ERROR_SOCKET   = 0x08,
    CTRL_ERROR_ARG_1    = 0x10,
    CTRL_ERROR_ARG_2    = 0x20,
    CTRL_ERROR_ARG_3    = 0x40,
}AR_CTRL_ERROR;

// Уровни серьёзности ошибок
typedef enum {
    ERR_LEVEL_DEBUG,
    ERR_LEVEL_INFO,
    ERR_LEVEL_WARNING,
    ERR_LEVEL_CRITICAL
} ErrorLevel;

// Структура ошибки
typedef struct {
    error_code_t code;          ///< Код ошибки
    ErrorLevel level;        ///< Уровень серьёзности
    time_t timestamp;        ///< Время возникновения
    const char *message;     ///< Текст ошибки
    const char *source_file; ///< Исходный файл
    uint32_t line_number;    ///< Номер строки
}ErrorEntry;

typedef enum
{
    cmd_idle    = 0,
    cmd_status  = 1,
    cmd_reboot  = 2,
    cmd_error   = 3,
    max_cmd     = 4
}CMD_TYPE_e;

typedef union
{
    struct
    {
        CMD_TYPE_e cmdType;
        uint8_t port;
    }msg_s;
    unsigned char arr[5];
}sock_msg_t;

typedef struct
{
    error_code_t errorCode;
    uint8_t status;
    uint8_t portNum;
    uint8_t resetCount;
    uint32_t time;
    uint32_t rx_speed_Kbit;
    TestType testType;
    char timeStr[32];
}resetPort_t;

typedef struct {
    AR_CTRL_ERROR arCtrlError;
    resetPort_t resetPort[NUM_PORTS];
}status_struct;

typedef  union
{
     status_struct status;
    //struct {
    //    AR_CTRL_ERROR arCtrlError;
    //    resetPort_t resetPort[NUM_PORTS];
    //}status;
    unsigned char arr[52*NUM_PORTS];
}resetPort_U;


#endif //TF_AR_POE_GLOBAL_INCLUDE_H
