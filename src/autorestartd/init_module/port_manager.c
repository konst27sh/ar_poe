//
// Created by sheverdin on 5/15/25.
//

#include "port_manager.h"
#include "config_loader.h"
#include <string.h>
#include <syslog.h>

static PortConfig portConfigs[NUM_PORTS];
static PortStatus portStatus[NUM_PORTS];
static RebootInfo rebootInfo[NUM_PORTS];

static PortFullInfo portData[NUM_PORTS];

static int parse_time_string(const char* time_str, time_h_m* out);

void port_manager_init(void)
{
    memset(portConfigs, 0, sizeof(portConfigs));
    memset(portStatus, 0, sizeof(portStatus));
    memset(rebootInfo, 0, sizeof(rebootInfo));

    for (uint8_t i = 0; i < NUM_PORTS; i++)
    {
        portConfigs[i].portNum = i + 1;
        portStatus[i].portState = PORT_STATE_DISABLED;
        portStatus[i].poeState = POE_STATE_OFF;
        rebootInfo[i] = (RebootInfo)
        {
            .rebootCount = 0,
            .lastRebootTime = time(NULL),
            .rebootDelay = 15 // Значение по умолчанию
        };
    }
    syslog(LOG_INFO, "Port manager initialized");
}

void port_manager_load_config(void)
{
    for (uint8_t i = 0; i < NUM_PORTS; i++)
    {
        PortConfig cfg =
        {
            .host = {'\0'},
            .timeAlarm = {0},
            .minSpeed = 0,
            .maxSpeed = 0
        };

        char section[16];
        snprintf(section, sizeof(section), "lan%d", i+1);
        config_load_section("tf_autorestart", section, CONFIG_SECTION_PORT);
        cfg.portNum = i+1;

        json_t *values = config_get_section(section);
        if (values) {
            // Mode
            json_t *mode = json_object_get(values, "mode");
            if (mode && json_is_string(mode)) {
                const char *mode_str = json_string_value(mode);
                if (strcmp(mode_str, "ping") == 0) cfg.testType = TEST_PING;
                else if (strcmp(mode_str, "link") == 0) cfg.testType = TEST_LINK;
                else if (strcmp(mode_str, "speed") == 0) cfg.testType = TEST_SPEED;
            }

            // Alarm
            json_t *alarm = json_object_get(values, "alarm");
            cfg.alarm = (alarm && json_is_string(alarm) &&
                         strcasecmp(json_string_value(alarm), "enable") == 0);

            // Host
            json_t *host = json_object_get(values, "host");
            if (host && json_is_string(host)) {
                strncpy(cfg.host, json_string_value(host), MAX_HOST_LEN-1);
            }

            // Speed
            json_t *min_speed = json_object_get(values, "min_speed");
            json_t *max_speed = json_object_get(values, "max_speed");
            if (min_speed && json_is_string(min_speed))
                cfg.minSpeed = atoi(json_string_value(min_speed));
            if (max_speed && json_is_string(max_speed))
                cfg.maxSpeed = atoi(json_string_value(max_speed));
        }

        port_manager_update_config(i, &cfg);
    }
}

void port_manager_update_config(uint8_t port_idx, const PortConfig* config)
{
    if (port_idx >= NUM_PORTS) return;

    memcpy(&portConfigs[port_idx], config, sizeof(PortConfig));

    syslog(LOG_DEBUG, "Port %d config: host=%s",
           port_idx + 1,
           portConfigs[port_idx].host);
}

void port_manager_init_reboot_info(void)
{
    for (uint8_t i = 0; i < NUM_PORTS; i++)
    {
        RebootInfo info =
        {
            .rebootCount = 0,
            .lastRebootTime = time(NULL),
            .rebootDelay = 15,
            .rebootState = REBOOT_STATE_IDLE
        };
        memcpy(&rebootInfo[i], &info, sizeof(RebootInfo));
    }
}

int port_manager_get_config(uint8_t portNum, PortConfig *config)
{
    if (portNum >= NUM_PORTS || config == NULL)
    {
        syslog(LOG_ERR, "Invalid port number or config pointer");
        return -1;
    }
    memcpy(config, &portConfigs[portNum], sizeof(PortConfig));
    return 0;
}

int port_manager_set_poe(uint8_t portNum, PoeState state)
{
    if (portNum >= NUM_PORTS) {
        syslog(LOG_ERR, "Invalid port number: %d", portNum);
        return -1;
    }

    // Аппаратно-зависимая реализация
    //if (poe_hardware_control(portNum + 1, state) != 0)
    //{
    //    syslog(LOG_ERR, "Failed to set PoE state for port %d", portNum + 1);
    //    return -1;
    //}

    portStatus[portNum].poeState = state;
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

static int poe_hardware_control(uint8_t port,  PoeState state)
{
    syslog(LOG_DEBUG, "PoE control stub: port %d -> %s",
           port, state == POE_STATE_ON ? "ON" : "OFF");
    return 0;
}

/**
 * @brief Логирует все инициализированные переменные системы
 */
void port_manager_log_state(void)
{
    syslog(LOG_DEBUG, "=== Port Manager State ===");
    for (uint8_t i = 0; i < NUM_PORTS; i++) {
        syslog(LOG_DEBUG, "Port %d: State=%s, PoE=%s",
               portConfigs[i].portNum,
               portStatus[i].portState == PORT_STATE_ENABLED ? "ENABLED" : "DISABLED",
               portStatus[i].poeState == POE_STATE_ON ? "ON" : "OFF");
    }
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

void port_handle_reboot(uint8_t portNum, uint8_t maxReset)
{
    if (rebootInfo[portNum].rebootCount >= maxReset)
    {
        syslog(LOG_ALERT, "Port %d: Maximum reboots reached", portNum + 1);
        port_manager_set_poe(portNum, POE_STATE_OFF);
        return;
    }

    // Логика перезагрузки
    port_manager_set_poe(portNum, POE_STATE_OFF);
    sleep(rebootInfo[portNum].rebootDelay);
    port_manager_set_poe(portNum, POE_STATE_ON);

    rebootInfo[portNum].rebootCount++;
    rebootInfo[portNum].lastRebootTime = time(NULL);
}

error_code_t port_run_test_ping(uint8_t portNum)
{
    if (portNum >= NUM_PORTS) return ERR_INVALID_PORT;

    // Пример реализации ping-теста
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "ping -c 1 %s", portData[portNum].config.host);
    int res = system(cmd);

    portData[portNum].testInfo.errorCode = (res == 0) ? ERR_OK : ERR_TEST_PING;
    return portData[portNum].testInfo.errorCode;
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

    portData[portNum].testInfo.errorCode =
            (speed >= portData[portNum].config.minSpeed &&
             speed <= portData[portNum].config.maxSpeed) ? ERR_OK : ERR_TEST_SPEED;

    return portData[portNum].testInfo.errorCode;
}

void port_manager_auto_reset(uint8_t portNum, error_code_t error, uint8_t maxReset)
{
    if (portNum >= NUM_PORTS) return;

    PortTestInfo *testInfo = &portData[portNum].testInfo;
    testInfo->errorCode = error;

    if (testInfo->resetCount >= maxReset) {
        syslog(LOG_ALERT, "Port %d: Max auto-reset attempts reached (Error: 0x%X)",
               portNum + 1, error);
        portData[portNum].status.portState = PORT_STATE_DISABLED;
        return;
    }

    // Обновление статистики
    testInfo->resetCount++;
    testInfo->totalResetCount++;
    portData[portNum].rebootInfo.rebootState = REBOOT_STATE_INIT;
}

AR_STATE port_manager_get_ar_state(uint8_t portNum)
{
    if (portNum >= NUM_PORTS) return IDLE_STATE;

    PortStatus status;
    if (port_manager_get_status(portNum, &status) != 0)
    {
        return IDLE_STATE;
    }

    // Проверка состояния порта и PoE
    if (status.portState != PORT_STATE_ENABLED)
    {
        return IDLE_STATE;
    }

    if (status.poeState != POE_STATE_ON)
    {
        return IDLE_STATE;
    }

    PortTestInfo *testInfo = &portData[portNum].testInfo;
    if (testInfo->errorCode != ERR_OK)
    {
        return REBOOT_STATE;
    }
    return REGULAR_STATE;
}

const char* test_type_to_str(TestType type)
{
    const char *names[] = {"disable", "link", "ping", "speed"};
    return (type < TEST_MAX) ? names[type] : "unknown";
}

int parse_time_string(const char* time_str, time_h_m* out) {
    int hours, minutes;
    if (sscanf(time_str, "%d:%d", &hours, &minutes) != 2) {
        return -1;
    }

    if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59) {
        return -1;
    }

    out->time_H = hours;
    out->time_M = minutes;
    out->status = BOOL_ENABLED;

    // Расчет абсолютного времени
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
        const PortConfig *cfg = &portConfigs[i];

        syslog(LOG_INFO, "Port %d:", i + 1);
        syslog(LOG_INFO, "  Mode: %s", test_type_to_str(cfg->testType));
        syslog(LOG_INFO, "  Alarm: %s", cfg->alarm ? "enabled" : "disabled");
        syslog(LOG_INFO, "  Host: %s", cfg->host);
        syslog(LOG_INFO, "  Speed: %u-%u Mbps", cfg->minSpeed, cfg->maxSpeed);

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








