//
// Created by sheverdin on 6/24/24.
//

#include "events_module.h"
#include "port_manager.h"
#include "init_module.h"
#include "init_module/error_handler.h"
#include "init_module/config_loader.h"
#include <time.h>

typedef  void (*eventHandler_t)(portInfo_t *portInfo, config_type *portConfigInfo);

static void rebootHandler(portInfo_t *portInfo, config_type *portConfigInfo);
static void test_ErrorHandler(portInfo_t *portInfo);
static void alarmHandler(portInfo_t *portInfo);

static eventHandler_t eventHandler[] =
{
    [IDLE_STATE]    = NULL,
    [REGULAR_STATE] = test_ErrorHandler,
    [REBOOT_STATE]  = rebootHandler
};

static void printResult(void);

//eventHandler_t eventHandler[MAX_AR_STATE] =
//{
//     NULL,
//     &test_ErrorHandler,
//     &rebootHandler
//};

void eventsHandler(void)
{
    for (uint8_t portIdx = 0; portIdx < NUM_PORTS; portIdx++)
    {
        portInfo_t portInfo;
        if (port_manager_get_info(portIdx, &portInfo) != 0) continue;

        if (eventHandler[portInfo.portResetInfo.state])
        {

            eventHandler[portInfo.portResetInfo.state](&portInfo);
        }

        if (portInfo.portResetInfo.event & MANUAL_RESTART) {
            if (portInfo.portStateInfo.port_state == PORT_STATE_ENABLED &&
                portInfo.portStateInfo.poe_state == PoE_STATE_ENABLED) {
                portInfo.portRebootInfo.rebootState = REBOOT_STATE_INIT;
                portInfo.portResetInfo.event &= ~MANUAL_RESTART;
            }
        }

        // Обновление конфигурации порта
        port_manager_update_info(portIdx, &portInfo);
    }
}

static void handleIdleState(portInfo_t *portInfo)
{
    if (portInfo->portResetInfo.event & TEST_OK)
    {
        portInfo->portResetInfo.event &= ~TEST_OK;
        portInfo->portResetInfo.state = REGULAR_STATE;
        portInfo->portResetInfo.errorCode &= ~ERR_PORT_SHUTDOWN;
    }
}



static void rebootHandler(portInfo_t *portInfo, config_type *portConfigInfo)
{
    const uint8_t maxReset = config_get_value("max_reset");

    switch (portInfo->portRebootInfo.rebootState)
    {
        case REBOOT_STATE_INIT:
            port_manager_poe_control(portInfo->portConfigInfo.portNum, POE_DOWN);
            portInfo->portRebootInfo.rebootTimeStart = time(NULL) +
                                                       portInfo->portRebootInfo.rebootDelay;
            portInfo->portRebootInfo.rebootState = REBOOT_STATE_POE_DOWN;
            break;

        case REBOOT_STATE_POE_DOWN:
            if (time(NULL) >= portInfo->portRebootInfo.rebootTimeStart)
            {
                port_manager_poe_control(portInfo->portConfigInfo.portNum, POE_UP);
                portInfo->portRebootInfo.rebootState = REBOOT_STATE_POE_UP;
                portInfo->portResetInfo.resetCount++;
            }
            break;

        case REBOOT_STATE_POE_UP:
            portInfo->portResetInfo.state = REGULAR_STATE;
            portInfo->portRebootInfo.rebootState = REBOOT_STATE_IDLE;
            portInfo->portResetInfo.event &= ~(AUTO_RESTART | MANUAL_RESTART);
            portInfo->portResetInfo.errorCode &= ~(ERR_REBOOTING | ERR_MANUAL_REBOOT);
            break;
    }

    if (portInfo->portResetInfo.resetCount >= maxReset)
    {
        error_register(ERR_PORT_SHUTDOWN, ERR_LEVEL_CRITICAL,
                       "Max reboot attempts", __FILE__, __LINE__);
        portInfo->portStateInfo.port_state = PORT_STATE_DISABLED;
    }
}


static void test_ErrorHandler(portInfo_t *portInfo)
{
    const uint8_t testNum = config_get_value("test_num");

    if (portInfo->portResetInfo.errorCode & ERR_OK) {
        portInfo->portResetInfo.errorTestCount =
                (portInfo->portResetInfo.errorTestCount > 0) ?
                portInfo->portResetInfo.errorTestCount - 1 : 0;
    } else if (portInfo->portResetInfo.errorCode & (
            ERR_TEST_LINK | ERR_TEST_PING | ERR_TEST_SPEED)) {

        if (++portInfo->portResetInfo.errorTestCount >= testNum) {
            portInfo->portResetInfo.event |= AUTO_RESTART;
            portInfo->portResetInfo.state = REBOOT_STATE;
        }
    }
}



static void printResult(void)
{
    portInfo_t *portInfoArr = port_get_portInfoArr();

    static uint8_t testCount = 0;
    static uint8_t errorCode = 0;

    for (int port = 0; port < NUM_PORTS; port++)
    {
        printf("-----------------  portConfigInfo[ %d ] - \n", port + 1);
        printf("portNum     = %d\n", portInfoArr[port].portConfigInfo.portNum);
        printf("test        = %d\n", portInfoArr[port].portConfigInfo.test);
        printf("errorCode   = %x\n", portInfoArr[port].portResetInfo.errorCode);
        printf("\n-----------------  \n");
        printf("\t\ttimeStr               = %s\n", portInfoArr[port].portConfigInfo.timeStr);
        printf("\t\talarm down        = %d\n", portInfoArr[port].portConfigInfo.timeAlarm[time_down].status);
        printf("\t\ttime remain down  = %d\n", portInfoArr[port].portConfigInfo.timeAlarm[time_down].remainTime);
        printf("\t\talarm up          = %d\n", portInfoArr[port].portConfigInfo.timeAlarm[time_up].status);
        printf("\t\ttime rimain up    = %d\n", portInfoArr[port].portConfigInfo.timeAlarm[time_up].remainTime);
        printf("\n-----------------  \n");

        if (portInfoArr[port].portConfigInfo.test == test_disable)
        {
            printf("test disable -- \n");
        } else if (portInfoArr[port].portConfigInfo.test == test_ping)
        {
            printf("reply_time      = %d\n", portInfoArr[port].portConfigInfo.reply_time);
        } else if (portInfoArr[port].portConfigInfo.test == test_speed)
        {
            printf("rx_speed_Kbit   = %d\n", portInfoArr[port].portConfigInfo.rx_speed_Kbit);
            printf("rx_delta        = %llu\n", portInfoArr[port].portConfigInfo.rx_delta_byte);
        }

        printf("test Count        = %d\n", portInfoArr[port].portResetInfo.errorTestCount);
        printf("reset Count       = %d\n", portInfoArr[port].portResetInfo.resetCount);
        printf("total Reset Count = %d\n", portInfoArr[port].portResetInfo.totalResetCount);
        printf("reset event       = %X\n", portInfoArr[port].portResetInfo.event);
        printf("reset state       = %d\n", portInfoArr[port].portResetInfo.state);
        printf("\n*********************************************\n");
        printf("reboot Event      = %x\n", portInfoArr[port].portRebootInfo.rebootEvent);
        printf("reboot State      = %d\n", portInfoArr[port].portRebootInfo.rebootState);
        printf("reboot TimeStart  = %d\n", portInfoArr[port].portRebootInfo.rebootTimeStart);
        printf("reboot Delay      = %d\n", portInfoArr[port].portRebootInfo.rebootDelay);
    }
    set_notRecivedMsg();
}
static void alarmHandler(portInfo_t *portInfo) {
    const time_t now = time(NULL);

    for (int i = 0; i < time_alarm_max; i++) {
        time_h_m *alarm = &portInfo->portConfigInfo.timeAlarm[i];

        if (alarm->status && now >= alarm->targetTime) {
            const PoeState state = (i == time_up) ? POE_UP : POE_DOWN;
            port_manager_poe_control(portInfo->portConfigInfo.portNum, state);

            if (i == time_down) {
                portInfo->portResetInfo.errorCode |= ERR_POE_DOWN;
            } else {
                portInfo->portResetInfo.errorCode &= ~ERR_POE_DOWN;
            }

            alarm->status = BOOL_DISABLED;
            port_manager_update_config(
                    portInfo->portConfigInfo.portNum - 1,
                    &portInfo->portConfigInfo
            );
        }
    }
}


