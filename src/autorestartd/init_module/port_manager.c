//
// Created by sheverdin on 5/15/25.
//

#include "port_manager.h"
#include "config_loader.h"
#include <string.h>
#include <syslog.h>

static port_config_info_t portConfigs[NUM_PORTS];
static PortStatus portStatus[NUM_PORTS];
static port_state_info_t portState[NUM_PORTS];
static port_reboot_info_t rebootInfo[NUM_PORTS];
static port_reset_info_t portResetInfo[NUM_PORTS];
static portInfo_t portData[NUM_PORTS];

static int parse_time_string(const char* time_str, time_h_m* out);  // todo

void port_manager_init(void)
{
    memset(portConfigs, 0, sizeof(portConfigs));
    memset(portState, 0, sizeof(portState));
    memset(rebootInfo, 0, sizeof(rebootInfo));

    for (uint8_t i = 0; i < NUM_PORTS; i++)
    {
        portConfigs[i].portNum = i + 1;
        portState[i].port_state = PORT_STATE_DISABLED;
        portState[i].poe_state = PoE_STATE_DISABLED;
        rebootInfo[i] = (port_reboot_info_t)
        {
            .rebootState     = REBOOT_STATE_IDLE,
            .rebootEvent     = REBOOT_EVENT_IDLE,
            .rebootDelay     = 15,
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

void port_manager_init_reboot_info(void)
{
    for (uint8_t i = 0; i < NUM_PORTS; i++)
    {
        port_reboot_info_t reboot_info =
        {
            .rebootState     = REBOOT_STATE_IDLE,
            .rebootEvent     = REBOOT_EVENT_IDLE,
            .rebootDelay     = 15,
            .rebootTimeStart = 0,
        };
        memcpy(&rebootInfo[i], &reboot_info, sizeof(port_reboot_info_t));
    }
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

int port_manager_get_status(uint8_t port_num, PortStatus *status)
{
    if (port_num >= NUM_PORTS || status == NULL)
    {
        syslog(LOG_ERR, "Invalid port number or status pointer");
        return -1;
    }
    memcpy(status, &portStatus[port_num], sizeof(PortStatus));
    return 0;
}

error_code_t port_run_test_link(uint8_t portNum)
{
    PortStatus status;
    if (port_manager_get_status(portNum, &status) != 0)
    {
        return ERR_PORT_DISABLE;
    }

    FILE *fp = fopen("/sys/class/net/lan1/operstate", "r");
    if (!fp) return ERR_HW_ACCESS;

    char state[16];
    fgets(state, sizeof(state), fp);
    fclose(fp);

    return (strcmp(state, "up\n") == 0) ? ERR_OK : ERR_TEST_LINK;
}

error_code_t port_run_test_ping(uint8_t portNum)
{
    if (portNum >= NUM_PORTS) return ERR_INVALID_PORT;

    char cmd[64];
    snprintf(cmd, sizeof(cmd), "ping -c 1 %s", portData[portNum].portConfigInfo.host);
    int res = system(cmd);

    portData[portNum].portResetInfo.errorCode = (res == 0) ? ERR_OK : ERR_TEST_PING;
    return portData[portNum].portResetInfo.errorCode;
}

error_code_t port_run_test_speed(uint8_t portNum)
{
    if (portNum >= NUM_PORTS) return ERR_INVALID_PORT;

    // Пример проверки скорости
    FILE *fp = fopen("/sys/class/net/lan1/speed", "r");
    if (!fp) return ERR_HW_ACCESS;

    uint32_t speed;
    fscanf(fp, "%u", &speed);
    fclose(fp);

    portData[portNum].portResetInfo.errorCode =
            (speed >= portData[portNum].portConfigInfo.minSpeed &&
             speed <= portData[portNum].portConfigInfo.maxSpeed) ? ERR_OK : ERR_TEST_SPEED;

    return portData[portNum].portResetInfo.errorCode;
}

void port_manager_auto_reset(uint8_t portNum, error_code_t error, uint8_t maxReset)
{
    if (portNum >= NUM_PORTS) return;

//PortTestInfo *portResetInfo_l = &portData[portNum].portResetInfo;
    portData[portNum].portResetInfo.errorCode = error;

    if (portData[portNum].portResetInfo.resetCount >= maxReset) {
        syslog(LOG_ALERT, "Port %d: Max auto-reset attempts reached (Error: 0x%X)",
               portNum + 1, error);
        portData[portNum].portStateInfo.port_state = PORT_STATE_DISABLED;
        return;
    }

    portData[portNum].portResetInfo.resetCount++;
    portData[portNum].portResetInfo.totalResetCount++;
    portData[portNum].portRebootInfo.rebootState = REBOOT_STATE_INIT;
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

        syslog(LOG_INFO, "Port     %d:", i + 1);
        syslog(LOG_INFO, "  Mode:  %s", test_type_to_str(cfg->testType));
        syslog(LOG_INFO, "  Alarm: %s", cfg->alarm ? "enabled" : "disabled");
        syslog(LOG_INFO, "  Host:  %s", cfg->host);
        syslog(LOG_INFO, "  Speed: %u - %u Mbps", cfg->minSpeed, cfg->maxSpeed);

        // Форматирование времени
        char time_buf[32];
        strftime(time_buf, sizeof(time_buf), "%H:%M",
                 localtime((const time_t *) &cfg->timeAlarm[time_up].targetTime));
        syslog(LOG_INFO, "  TimeUp: %s", time_buf);

        strftime(time_buf, sizeof(time_buf), "%H:%M",
                 localtime((const time_t *) &cfg->timeAlarm[time_down].targetTime));
        syslog(LOG_INFO, "  TimeDown: %s", time_buf);
    }
}








