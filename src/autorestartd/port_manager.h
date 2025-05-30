//
// Created by sheverdin on 5/16/25.
//

#ifndef TF_AUTORESTART_PORT_MANAGER_H
#define TF_AUTORESTART_PORT_MANAGER_H

/**
 * @file port_manager.h
 * @brief Управление параметрами и состоянием PoE-портов
 */

#include <stdint.h>
#include <time.h>
#include "../global_includs.h"


#define MAX_HOST_LEN      16
#define MAX_TIME_STR_LEN  32
#define MAX_MODE_LEN 	  16

typedef enum
{
    IDLE_STATE       = 0,
    REGULAR_STATE    = 1,
    REBOOT_STATE     = 2,
    MAX_AR_STATE
}AR_STATE;

typedef enum
{
    IDLE_EVENT       = 0x00,
    TEST_OK          = 0x01,
    AUTO_RESTART     = 0x02,
    MANUAL_RESTART   = 0x04,
}AR_EVENT;

typedef enum
{
    REBOOT_STATE_IDLE        = 0,
    REBOOT_STATE_INIT        = 1,
    REBOOT_STATE_POE_DOWN    = 2,
    REBOOT_STATE_POE_UP      = 3
}REBOOT_STATE_e;

typedef enum
{
    REBOOT_EVENT_IDLE         = 0x00,
    REBOOT_EVENT_START        = 0x01,
    REBOOT_EVENT_TIMER_STOP   = 0x02,
}REBOOT_EVENT_e;


typedef enum
{
    PORT_STATE_DISABLED,
    PORT_STATE_ENABLED
}PortState;

typedef enum {
    PoE_STATE_DISABLED,
    PoE_STATE_ENABLED
}PoeState;

typedef enum
{
    time_up         = 0,
    time_down       = 1,
    time_alarm_max  = 2
}TIME_ALARM_INDEX_e;

typedef struct
{
    bool_t  status;
    uint8_t time_H;
    uint8_t time_M;
    uint32_t targetTime;
    uint32_t remainTime;
}time_h_m;

typedef struct
{
    uint8_t portNum;
    uint8_t isInit;
    TestType testType;
    char mode[MAX_MODE_LEN];
    bool_t alarm;
    char host[MAX_HOST_LEN];
    uint32_t minSpeed;
    uint32_t maxSpeed;
    unsigned long long       rx_byte_prev;
    unsigned long long       rx_byte_current;
    unsigned long long       rx_delta_byte;
    uint32_t        rx_speed_Kbit;
    uint32_t        tx_byte_prev;
    uint32_t        time_current;
    uint32_t        time_prev;

    time_h_m timeAlarm[time_alarm_max];
    char            timeStr[32];
}port_config_info_t;

typedef struct
{
    uint8_t         resetCount;
    uint8_t         totalResetCount;
    uint8_t         errorTestCount;
    error_code_t    errorCode;
    AR_EVENT        event;
    AR_STATE        state;
}port_reset_info_t;

typedef struct
{
    bool_t         linkState;
    PortState      port_state;
    PoeState       poe_state;
}port_state_info_t;

typedef struct
{
    PortState   portState;
    PoeState    poeState;
    uint32_t    rxBytes;
    uint32_t    txBytes;
}PortStatus;

#pragma pack(push,1)
typedef struct
{
    REBOOT_STATE_e  rebootState;
    REBOOT_EVENT_e  rebootEvent;
    uint8_t         rebootDelay;
}port_reboot_info_t;
#pragma pack(pop)

typedef enum
{
    POE_UP      = 0x01,
    POE_DOWN    = 0x02,
    POE_RESTART = 0x03
}POE_CONTROL;

typedef struct
{
    port_config_info_t  portConfigInfo;
    port_reset_info_t   portResetInfo;
    port_state_info_t   portStateInfo;
    port_reboot_info_t  portRebootInfo;
}portInfo_t;

/**
 * @brief Инициализирует все порты
 */
void port_manager_init();

/**
 * @brief Получает конфигурацию порта
 * @param portNum Номер порта (0-based)
 * @param[out] config Буфер для конфигурации
 * @return 0 при успехе, -1 при ошибке
 */

int port_manager_get_config(uint8_t portNum, port_config_info_t *config);

/**
 * @brief Обновляет статистику порта
 * @param portNum Номер порта (0-based)
 */
void port_manager_load_config(void);
void port_manager_update_config(uint8_t port_idx, const port_config_info_t* config);

void port_manager_auto_reset(uint8_t portNum, uint8_t maxReset);
AR_STATE port_manager_get_ar_state(uint8_t portNum);

void port_run_test_link(uint8_t portNum);
void port_run_test_ping(uint8_t portNum);
void port_run_test_speed(uint8_t portNum);
void port_run_test_disable(uint8_t portNum);

const char* test_type_to_str(TestType type);
void port_manager_log_all_configs(void);

int port_manager_get_info(uint8_t portIdx, portInfo_t *info);
int port_manager_poe_control(uint8_t portNum, POE_CONTROL state);
int port_manager_update_info(uint8_t portIdx, const portInfo_t *info);
void autoResetHandler(uint8_t portNum, uint8_t maxReset,  portInfo_t *portInfo);
void manualResetHandler(uint8_t portNum, portInfo_t *portInfo);
uint32_t set_timeStart(portInfo_t *portInfo);
REBOOT_EVENT_e get_rebootEvent(portInfo_t *portInfo);


#endif //TF_AUTORESTART_PORT_MANAGER_H
