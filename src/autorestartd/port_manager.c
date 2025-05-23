//
// Created by sheverdin on 5/15/25.
//

#include "port_manager.h"
#include "init_module/config_loader.h"
#include <string.h>
#include <syslog.h>
#include "ar_ping.h"
#include "init_module/error_handler.h"
#include "utils_lib/utils_module.h"

static port_config_info_t portConfigs[NUM_PORTS];
static port_state_info_t portState[NUM_PORTS];
static port_reboot_info_t portRebootInfo[NUM_PORTS];
static port_reset_info_t portResetInfo[NUM_PORTS];

static int parse_time_string(const char* time_str, time_h_m* out);  // todo
void set_errorHandler(uint8_t index, portInfo_t *portInfo);

void port_manager_init(void)
{
    memset(portConfigs, 0, sizeof(port_config_info_t));
    memset(portState, 0, sizeof(port_state_info_t));
    memset(portRebootInfo, 0, sizeof(port_reboot_info_t));
    memset(portResetInfo, 0, sizeof(port_reset_info_t));

    for (uint8_t i = 0; i < NUM_PORTS; i++)
    {
        portConfigs[i] = (port_config_info_t)
        {
            .portNum            = i + 1,
            .isInit             = 1,
            .time_current       = time(NULL),
            .rx_delta_byte      = 0,
            .rx_byte_prev       = 0,
            .rx_byte_current    = 0,
            .rx_speed_Kbit      = 0,
            .time_prev          = 0,
        };

        portState[i] = (port_state_info_t)
        {
            .port_state = PORT_STATE_DISABLED,
            .poe_state = PoE_STATE_DISABLED,
        };

        portResetInfo[i] = (port_reset_info_t)
        {
            .errorCode        = ERR_OK,
            .resetCount       = 0,
            .errorTestCount   = 0,
            .state            = REGULAR_STATE,
            .event            = IDLE_EVENT,
        };

        portRebootInfo[i] = (port_reboot_info_t)
        {
            .rebootState     = REBOOT_STATE_IDLE,
            .rebootEvent     = REBOOT_EVENT_IDLE,
            .rebootDelay     = 11,
            .rebootTimeStart = 0,
        };
    }
    syslog(LOG_INFO, "Port manager initialized");
}

void port_manager_load_config(void)
{
    for (uint8_t i = 0; i < NUM_PORTS; i++)
    {
        char section[16];
        snprintf(section, sizeof(section), "lan%d", i+1);

        json_t *root = config_load_section("tf_autorestart", section);
        if (root == NULL)
        {
            syslog(LOG_ERR, "json root not valid");
            continue;
        }

        json_t *values = json_object_get(root, "values");
        if (values == NULL)
        {
            syslog(LOG_ERR, "values not valid");
            json_decref(root);
            continue;
        }

        port_config_info_t cfg = {0};
        cfg.portNum = i + 1;

        if (values != NULL)
        {
            // Mode
            json_t *mode = json_object_get(values, "mode");
            if (mode != NULL && json_is_string(mode))
            {
                const char *mode_str = json_string_value(mode);
                if (strcmp(mode_str, "ping") == 0) cfg.testType = TEST_PING;
                else if (strcmp(mode_str, "link") == 0) cfg.testType = TEST_LINK;
                else if (strcmp(mode_str, "speed") == 0) cfg.testType = TEST_SPEED;
            }

            /// Alarm
            json_t *alarm = json_object_get(values, "alarm");
            cfg.alarm = (alarm != NULL && json_is_string(alarm) && strcasecmp(json_string_value(alarm), "enable") == 0);

            /// Host
            json_t *host = json_object_get(values, "host");
            if (host != NULL && json_is_string(host))
            {
                strncpy(cfg.host, json_string_value(host), MAX_HOST_LEN-1);
            }

            /// Speed
            json_t *min_speed = json_object_get(values, "min_speed");
            json_t *max_speed = json_object_get(values, "max_speed");
            if (min_speed && json_is_string(min_speed))
            {
                cfg.minSpeed = atoi(json_string_value(min_speed));
            }
            if (max_speed && json_is_string(max_speed))
            {
                cfg.maxSpeed = atoi(json_string_value(max_speed));
            }
        }
        port_manager_update_config(i, &cfg);
        json_decref(root);
    }
}

void port_manager_update_config(uint8_t port_idx, const port_config_info_t *config)
{
    if (port_idx >= NUM_PORTS)
        return;
    portConfigs[port_idx] = *config;
}

int port_manager_get_config(uint8_t portNum, port_config_info_t *config)
{
    if (portNum >= NUM_PORTS || config == NULL)
    {
        syslog(LOG_ERR, "Invalid port number or portConfigInfo pointer");
        return -1;
    }
    memcpy(config, &portConfigs[portNum], sizeof(port_config_info_t));
    return 0;
}

void port_run_test_disable(uint8_t portNum)
{
    uint8_t index = portNum + 1;
    if (index > NUM_PORTS)
    {
        portResetInfo[portNum].errorCode |= ERR_INVALID_PORT;
        return;
    }
    portResetInfo[portNum].errorCode &= (~ERR_OK);
    portResetInfo[portNum].errorCode |= (ERR_TEST_DISABLE);
}

void port_run_test_link(uint8_t portNum)
{
    uint8_t index = portNum + 1;
    if (index > NUM_PORTS)
    {
        syslog(LOG_CRIT, "test_link -> index > NUM_PORTS");
        return;
    }

    char linkTest_cmd[64];
    snprintf(linkTest_cmd, sizeof(linkTest_cmd), "/sys/class/net/lan%d/operstate", index);
    FILE *fp = fopen(linkTest_cmd, "r");
    if (!fp)
    {
        portResetInfo[portNum].errorCode &= (~ERR_OK);
        portResetInfo[portNum].errorCode |= (ERR_HW_ACCESS);
    }

    char linkState[16];
    fgets(linkState, sizeof(linkState), fp);
    fclose(fp);
    linkState[strcspn(linkState, "\r\n")] = '\0';
    if (strstr(linkState, "up") != NULL)
    {
        portResetInfo[portNum].errorCode |= ERR_OK;
        portResetInfo[portNum].errorCode &= (~ERR_TEST_LINK);
    }
    else
    {
        portResetInfo[portNum].errorCode |= ERR_TEST_LINK;
        portResetInfo[portNum].errorCode &= (~ERR_OK);
    }

    syslog(LOG_INFO, "port_run_test_link  state = %s -- port = %d -- errorCode = %X", linkState, index, portResetInfo[portNum].errorCode);
}

void port_run_test_ping(uint8_t portNum)
{
    uint8_t index = portNum + 1;
    if (index > NUM_PORTS)
    {
        syslog(LOG_CRIT, "test_ping -> index > NUM_PORTS");
        return;
    }

    portResetInfo[portNum].errorCode &= (~ERR_OK);

    if (portResetInfo[portNum].errorCode & ERR_TEST_LINK)
        return;

    syslog(LOG_INFO, "ping host = %s", portConfigs[portNum].host);
    ping_error_t ping_res = ping_host(portConfigs[portNum].host);
    if (ping_res == PING_OK)
    {
        portResetInfo[portNum].errorCode |=ERR_OK;
    }
    else
    {
        portResetInfo[portNum].errorCode |= ERR_TEST_PING;
    }

    syslog(LOG_INFO, "port_run_test_ping  state = %d -- port = %d -- error = %x", ping_res, index, portResetInfo[portNum].errorCode);
}

void port_run_test_speed(uint8_t portNum)
{
    uint8_t index = portNum + 1;

    if (index > NUM_PORTS)
    {
        syslog(LOG_CRIT, "test_speed -> index > NUM_PORTS");
        return;
    }
    portResetInfo[portNum].errorCode &= (~ERR_OK);

    if (portResetInfo[portNum].errorCode & ERR_TEST_LINK)
        return;

    json_t *root = config_load_speed(index);
    if (root == NULL)
    {
        syslog(LOG_ERR, "json root not valid");
        portResetInfo[portNum].errorCode |= ERR_TEST_SPEED;
        return;
    }
    json_t *values = json_object_get(root, "nic_stat");
    if (values == NULL)
    {
        syslog(LOG_ERR, "values not valid");
        json_decref(root);
        portResetInfo[portNum].errorCode |= ERR_TEST_SPEED;
        return;
    }
    char value_str[MAX_PARAM_VALUE] = {0};

    if (!json_is_string(values))
    {
        portResetInfo[portNum].errorCode |= ERR_TEST_SPEED;
    }

    strncpy(value_str, json_string_value(values), MAX_PARAM_VALUE - 1);
    value_str[MAX_PARAM_VALUE - 1] = '\0';


    portResetInfo[portNum].errorCode &= (~ERR_TEST_SPEED);
    portResetInfo[portNum].errorCode |= ERR_OK;

    int rx_byte_current = (int) strtoll(value_str, NULL, 10);
    uint32_t delta_time = 0;

    portConfigs[portNum].rx_delta_byte =  portConfigs[portNum].rx_byte_current  -  portConfigs[portNum].rx_byte_prev;
    portConfigs[portNum].rx_byte_prev =  portConfigs[portNum].rx_byte_current;
    portConfigs[portNum].time_current = time(NULL);
    portConfigs[portNum].time_prev =  portConfigs[portNum].time_current;
    delta_time =  portConfigs[portNum].time_current -  portConfigs[portNum].time_prev;

    if (delta_time == 0)
    {
        portResetInfo[portNum].errorCode &= (~ERR_OK);
        portResetInfo[portNum].errorCode |= ERR_TEST_SPEED;
    }
    else
    {
        portConfigs[portNum].rx_speed_Kbit = portConfigs[portNum].rx_delta_byte / (delta_time);
        portConfigs[portNum].rx_speed_Kbit =  portConfigs[portNum].rx_speed_Kbit >> 7;

        if ((portConfigs[portNum].rx_speed_Kbit  < portConfigs[portNum].minSpeed) ||
            (portConfigs[portNum].rx_speed_Kbit  > portConfigs[portNum].maxSpeed))
        {
            portResetInfo[portNum].errorCode &= (~ERR_OK);
            portResetInfo[portNum].errorCode |= ERR_TEST_SPEED;
        }
       //else
       //{
       //    // todo speed test
       //    if (portResetInfo[portNum].state == IDLE_STATE)
       //    {
       //        portResetInfo[portNum].event |= TEST_OK;
       //    }
       //}
    }
    syslog(LOG_INFO, "speed test: speed %u -- minSpeed = %d -- maxSpeed = %d -- error = %x",
           rx_byte_current, portConfigs[portNum].minSpeed,  portConfigs[portNum].maxSpeed, portResetInfo[portNum].errorCode);
}

void port_manager_auto_reset(uint8_t portNum, uint8_t maxReset)
{
    if (portNum >= NUM_PORTS)
        return;

     portResetInfo[portNum].errorCode;

    if (portResetInfo[portNum].resetCount >= maxReset)
    {
        syslog(LOG_ALERT, "Port %d: Max auto-reset attempts reached (Error: 0x%X)",
               portNum + 1, portResetInfo[portNum].errorCode);
        portState[portNum].port_state = PORT_STATE_DISABLED;
        return;
    }

    portResetInfo[portNum].resetCount++;
    portResetInfo[portNum].totalResetCount++;
    portRebootInfo[portNum].rebootState = REBOOT_STATE_INIT;
}

AR_STATE port_manager_get_ar_state(uint8_t portNum)
{
    portState[portNum-1].poe_state = PoE_STATE_DISABLED;
    portState[portNum-1].port_state = PORT_STATE_DISABLED;
    portResetInfo[portNum-1].errorCode  &= (~ERR_OK);
    portResetInfo[portNum-1].errorCode |= ERR_PORT_DISABLE;
    portResetInfo[portNum-1].errorCode |= ERR_PoE_DISABLE;
    portResetInfo[portNum-1].state = IDLE_STATE;
    AR_STATE ar_state = portResetInfo[portNum-1].state;

    if (portNum >= NUM_PORTS)
        return ar_state;

    char section[16];
    snprintf(section, sizeof(section), "lan%d", portNum+1);

    json_t *root = config_load_section("port", section);
    if (root == NULL)
    {
        syslog(LOG_ERR, "json root not valid");
        return ar_state;
    }

    json_t *values = json_object_get(root, "values");
    if (values == NULL)
    {
        syslog(LOG_ERR, "values not valid");
        json_decref(root);
        return ar_state;
    }

    char value_str[MAX_PARAM_VALUE] = {0};

    if (parse_json_param(values, "state", value_str) == 0)
    {
        if (strcmp (value_str, "enable") == 0)
        {
            portState[portNum-1].port_state = PORT_STATE_ENABLED;
            portResetInfo[portNum-1].errorCode  &= (~ERR_PORT_DISABLE);
            ar_state = REGULAR_STATE;
        }
    }

    if (parse_json_param(values, "poe", value_str) == 0)
    {
        if (strcmp (value_str, "enable") ==0)
        {
            portState[portNum-1].poe_state = PoE_STATE_ENABLED;
            portResetInfo[portNum-1].errorCode  &= (~ERR_PoE_DISABLE);
            ar_state = REGULAR_STATE;
        }
    }

    return ar_state;
}

const char* test_type_to_str(TestType type)
{
    const char *names[] = {"disable", "link", "ping", "speed"};
    return (type < TEST_MAX) ? names[type] : "unknown";
}

static int parse_time_string(const char* time_str, time_h_m* out)
{
    int hours, minutes;
    if (sscanf(time_str, "%d:%d", &hours, &minutes) != 2)
    {
        return -1;
    }

    if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59) {
        return -1;
    }

    out->time_H = hours;
    out->time_M = minutes;
    out->status = BOOL_ENABLED;

    time_t now = time(NULL);
    struct tm *tm = localtime(&now);
    tm->tm_hour = hours;
    tm->tm_min = minutes;
    tm->tm_sec = 0;
    out->targetTime = mktime(tm);

    return 0;
}

void port_manager_log_all_configs(void)
{
    syslog(LOG_INFO, "=== Ports Configuration ===");

    for (uint8_t i = 0; i < NUM_PORTS; i++)
    {
        const port_config_info_t *cfg = &portConfigs[i];
        // Форматирование времени
        char timeUp_buf[32];
        char timeDown_buf[32];
        strftime(timeUp_buf, sizeof(timeUp_buf), "%H:%M",
                 localtime((const time_t *) &cfg->timeAlarm[time_up].targetTime));
        strftime(timeDown_buf, sizeof(timeDown_buf), "%H:%M",
                 localtime((const time_t *) &cfg->timeAlarm[time_down].targetTime));
        syslog(LOG_INFO, "Port: %d -- Mode: %s -- Alarm: %s -- Host:  %s -- Speed: %u - %u Mbps -- TimeUp: %s -- TimeDown: %s",
               i + 1, test_type_to_str(cfg->testType), cfg->alarm ? "enabled" : "disabled", cfg->host,  cfg->minSpeed, cfg->maxSpeed, timeUp_buf, timeDown_buf);
    }
}

int port_manager_poe_control(uint8_t portNum, POE_CONTROL state)
{
    // Реализация управления PoE через аппаратный API
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "poe_ctrl %d %s",
             portNum, (state == POE_UP) ? "up" : "down");
    return system(cmd);
}

int port_manager_get_info(uint8_t portIdx, portInfo_t *info)
{
    if (portIdx >= NUM_PORTS) return -1;

    // Копируем каждую структуру отдельно
    memcpy(&info->portConfigInfo, &portConfigs[portIdx], sizeof(port_config_info_t));
    memcpy(&info->portResetInfo, &portResetInfo[portIdx], sizeof(port_reset_info_t));
    memcpy(&info->portRebootInfo, &portRebootInfo[portIdx], sizeof(port_reboot_info_t));
    memcpy(&info->portStateInfo, &portState[portIdx], sizeof(port_state_info_t));

    printf("get_info  1 portIdx =  %d -- rebootDelay = %u -- timeStart = %lu\n",
           portIdx, portRebootInfo[portIdx].rebootDelay, portRebootInfo[portIdx].rebootTimeStart);
    printf("get_info  2 portIdx =  %d -- rebootDelay = %u -- timeStart = %lu\n",
           portIdx, info->portRebootInfo.rebootDelay, info->portRebootInfo.rebootTimeStart);
    return 0;
}

int port_manager_update_info(uint8_t portIdx, const portInfo_t *info) {
    if (portIdx >= NUM_PORTS || info == NULL) {
        error_register(ERR_UNAVAILABLE_RESOURCE, ERR_LEVEL_WARNING,
                       "Invalid port index in update_info", __FILE__, __LINE__);
        return -1;
    }

    memcpy(&portConfigs[portIdx], &info->portConfigInfo, sizeof(port_config_info_t));
    memcpy(&portResetInfo[portIdx], &info->portResetInfo, sizeof(port_reset_info_t));
    memcpy(&portRebootInfo[portIdx], &info->portRebootInfo, sizeof(port_reboot_info_t));
    memcpy(&portState[portIdx], &info->portStateInfo, sizeof(port_state_info_t));

    printf("update_info 1 portIdx =  %d -- rebootDelay = %u -- timeStart = %lu\n",
           portIdx, info->portRebootInfo.rebootDelay, info->portRebootInfo.rebootTimeStart);
    printf("update_info 2 portIdx =  %d -- rebootDelay = %u -- timeStart = %lu\n",
           portIdx, portRebootInfo[portIdx].rebootDelay, portRebootInfo[portIdx].rebootTimeStart);
    return 0;
}

void autoResetHandler(uint8_t portNum, uint8_t maxReset,  portInfo_t *portInfo)
{
    uint8_t index = portNum - 1;
    if (index >= NUM_PORTS) return;

    // Обновление времени последнего сброса
    char timestamp[32];
    time_error_t timeError = getTimeDate(timestamp, sizeof(timestamp));
    printf("index = %d\n", index);
    printf("timestamp = %s\n", timestamp);

    switch (timeError)
    {
        case TIME_ERR_SYSYTEM:
            printf("Failed to get system time\n");
            error_register(ERR_TIME_NOT_VALID, ERR_LEVEL_WARNING,
                           "Failed to get system time", __FILE__, __LINE__);
            break;
        case TIME_ERR_CONVERT:
            printf("Failed to convert local time\n");
            error_register(ERR_TIME_NOT_VALID, ERR_LEVEL_WARNING,
                           "Failed to convert local time", __FILE__, __LINE__);
            break;
        case TIME_ERR_BUFFER:
            printf("ime buffer overflow\n");
            error_register(ERR_NULL_OBJ, ERR_LEVEL_WARNING,
                           "Time buffer overflow", __FILE__, __LINE__);
            break;
    }

    strncpy(portInfo[index].portConfigInfo.timeStr, timestamp, 31);

    portInfo[index].portResetInfo.errorCode &= (~ERR_OK);

    printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>   resetCount = %d\n", portInfo[index].portResetInfo.resetCount);
    if (portInfo[index].portResetInfo.resetCount >= maxReset)
    {
        portInfo[index].portResetInfo.errorCode &= (~ERR_REBOOTING);
        portInfo[index].portResetInfo.errorCode |= ERR_PORT_SHUTDOWN;
        portInfo[index].portRebootInfo.rebootState =  REBOOT_STATE_IDLE;
        portInfo[index].portResetInfo.state = IDLE_STATE;
        syslog(LOG_INFO, "Reboot disable after max reset, port - %d", portNum);
        error_register(ERR_PORT_SHUTDOWN, ERR_LEVEL_WARNING,
                       "Auto-reboot disabled (max attempts)", __FILE__, __LINE__);
        set_errorHandler(index, portInfo);
    }
    else
    {
        syslog(LOG_INFO, "auto reset port = %d", portNum);
        syslog(LOG_INFO, "total reset count = %d", portInfo[index].portResetInfo.totalResetCount);
        portInfo[index].portResetInfo.errorTestCount = 0;
        portInfo[index].portResetInfo.resetCount++;
        portInfo[index].portResetInfo.totalResetCount++;
        portInfo[index].portResetInfo.errorCode |= ERR_REBOOTING;
        portInfo[index].portRebootInfo.rebootState = REBOOT_STATE_INIT;
    }
}

void manualResetHandler(uint8_t portNum, portInfo_t *portInfo)
{
    uint8_t index = portNum - 1;
    if (index >= NUM_PORTS) return;

    error_register(ERR_OK, ERR_LEVEL_INFO,
                   "Manual reboot triggered", __FILE__, __LINE__);

    portInfo[index].portResetInfo.resetCount = 0;
    portInfo[index].portResetInfo.errorTestCount = 0;
    portInfo[index].portResetInfo.totalResetCount++;
    portInfo[index].portRebootInfo.rebootState = REBOOT_STATE_INIT;
}

void set_errorHandler(uint8_t index, portInfo_t *portInfo)
{
    if (
            portInfo[index].portResetInfo.errorCode      & ERR_IP_NOT_VALID
            || portInfo[index].portResetInfo.errorCode   & ERR_TIME_NOT_VALID
            || portInfo[index].portResetInfo.errorCode   & ERR_SPEED_VALUE
            || portInfo[index].portResetInfo.errorCode   & ERR_PORT_DISABLE
            )
    {
        portInfo[index].portConfigInfo.testType = TEST_DISABLE;
    }
}

time_t set_timeStart(uint8_t portnum)
{
    uint8_t portIndex = portnum - 1;
    portRebootInfo[portIndex].rebootTimeStart = (time_t) (time(NULL) + portRebootInfo[portIndex].rebootDelay);
    printf("rebootDelay = %d  port = %d\n",portRebootInfo[portIndex].rebootDelay, portIndex);
    printf("time now = %ld -- rebootTimeStart = %lu\n", time(NULL), portRebootInfo[portIndex].rebootTimeStart);
    return portRebootInfo[portIndex].rebootTimeStart;
}

REBOOT_EVENT_e get_rebootEvent(uint8_t portnum)
{
    uint8_t portIndex = portnum - 1;
    if (time(NULL) < portRebootInfo[portIndex].rebootTimeStart)
    {
        time_t delta = portRebootInfo[portIndex].rebootTimeStart - time(NULL);
        printf("TIME REMAIN %ld\n", delta);
        printf("---> time now = %ld -- rebootTimeStart = %ld\n", time(NULL),
                portRebootInfo[portIndex].rebootTimeStart);
    }
    if (time(NULL) >= portRebootInfo[portIndex].rebootTimeStart)
    {
        printf("EVENT ---> time now = %ld -- rebootTimeStart = %lu\n", (unsigned long) time(NULL),
               (unsigned long) portRebootInfo[portIndex].rebootTimeStart);
        return REBOOT_EVENT_TIMER_STOP;
    }
    else
    {
        return REBOOT_EVENT_IDLE;
    }
}

