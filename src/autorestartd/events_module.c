//
// Created by sheverdin on 6/24/24.
//

#include "events_module.h"
#include "port_manager.h"
#include "init_module.h"
#include "init_module/error_handler.h"
#include "init_module/config_loader.h"
#include <time.h>

typedef  void (*eventHandler_t)(portInfo_t *portInfo);

static void rebootHandler(portInfo_t *portInfo);
static void test_ErrorHandler(portInfo_t *portInfo);
static void alarmHandler(portInfo_t *portInfo);
static void handleIdleState(portInfo_t *portInfo);


static eventHandler_t eventHandler[] =
{
    [IDLE_STATE]    = handleIdleState,
    [REGULAR_STATE] = test_ErrorHandler,
    [REBOOT_STATE]  = rebootHandler
};

static void printResult(portInfo_t *portInfo);

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

        syslog(LOG_DEBUG, "port = %d -- State = %x -- %d", portIdx + 1, portInfo.portResetInfo.state, portInfo.portResetInfo.state);
        printf("====================================== rebootDelay = %d\n", portInfo.portRebootInfo.rebootDelay);
        if (eventHandler[portInfo.portResetInfo.state])
        {
            eventHandler[portInfo.portResetInfo.state](&portInfo);
        }

        if (portInfo.portResetInfo.event & MANUAL_RESTART)
        {
            if (portInfo.portStateInfo.port_state == PORT_STATE_ENABLED &&
                portInfo.portStateInfo.poe_state == PoE_STATE_ENABLED)
            {
                portInfo.portRebootInfo.rebootState = REBOOT_STATE_INIT;
                portInfo.portResetInfo.event &= ~MANUAL_RESTART;
            }
        }

        if (portInfo.portConfigInfo.alarm)
        {
            alarmHandler(&portInfo);
        }

        //printResult(&portInfo);

        port_manager_update_info(portIdx, &portInfo);
    }
}

static void handleIdleState(portInfo_t *portInfo)
{
    syslog(LOG_DEBUG, "handleIdleState");
    if (portInfo->portResetInfo.event & TEST_OK)
    {
        portInfo->portResetInfo.event &= ~TEST_OK;
        portInfo->portResetInfo.state = REGULAR_STATE;
        portInfo->portResetInfo.errorCode &= ~ERR_PORT_SHUTDOWN;

        error_register(ERR_OK, ERR_LEVEL_DEBUG,
                       "IDLE -> REGULAR: TEST_OK processed", __FILE__, __LINE__);
    }
}

static void rebootHandler(portInfo_t *portInfo)
{
    const uint8_t maxReset = config_get_value("max_reset");
    syslog(LOG_DEBUG, "rebootHandler maxReset = %d", maxReset);

    switch (portInfo->portRebootInfo.rebootState)
    {
        case REBOOT_STATE_IDLE:
            if (portInfo->portResetInfo.event & AUTO_RESTART)
            {
                portInfo->portResetInfo.event &= (~AUTO_RESTART);
                autoResetHandler(portInfo->portConfigInfo.portNum, maxReset, portInfo);
            }
            if (portInfo->portResetInfo.event & MANUAL_RESTART)
            {
                if (portInfo->portStateInfo.port_state == PORT_STATE_ENABLED &&
                    portInfo->portStateInfo.poe_state == PoE_STATE_ENABLED)
                {
                    manualResetHandler(portInfo->portConfigInfo.portNum, portInfo);
                    portInfo->portResetInfo.event &= ~MANUAL_RESTART;
                }
            }
            break;

        case REBOOT_STATE_INIT:
            portInfo->portRebootInfo.rebootState = REBOOT_STATE_POE_DOWN;
            portInfo->portRebootInfo.rebootEvent |= REBOOT_EVENT_START;
            break;

        case REBOOT_STATE_POE_DOWN:
            if (portInfo->portRebootInfo.rebootEvent & REBOOT_EVENT_START)
            {
                portInfo->portRebootInfo.rebootEvent &= (~REBOOT_EVENT_START);
                //poe_Control(portNum, POE_DOWN);
                syslog(LOG_INFO, "REBOOT STATE POE_DOWN , port - %d", portInfo->portConfigInfo.portNum);
                portInfo->portRebootInfo.rebootTimeStart = set_timeStart(portInfo->portConfigInfo.portNum);
            }

            portInfo->portRebootInfo.rebootEvent = get_rebootEvent(portInfo->portConfigInfo.portNum);

            if (portInfo->portRebootInfo.rebootEvent & REBOOT_EVENT_TIMER_STOP)
            {
                syslog(LOG_INFO, "REBOOT STATE POE_DOWN , port - %d", portInfo->portConfigInfo.portNum);
                printf("REBOOT_EVENT_TIMER_STOP\n");
                portInfo->portRebootInfo.rebootState = REBOOT_STATE_POE_UP;
            }
            break;

        case REBOOT_STATE_POE_UP:
            portInfo->portRebootInfo.rebootEvent &= (~REBOOT_EVENT_TIMER_STOP);
            openlog("tf_autoresart", LOG_PID, LOG_USER);
            if (portInfo->portStateInfo.poe_state == PoE_STATE_ENABLED && portInfo->portStateInfo.port_state == PORT_STATE_ENABLED)
            {
                //poe_Control(portNum, POE_UP);
                syslog(LOG_INFO, "POE UP - DONE, port - %d\n", portInfo->portConfigInfo.portNum);
            }
            else
            {
                syslog(LOG_INFO, "POE UP - PoE in state disable %d\n", portInfo->portConfigInfo.portNum);
            }
            portInfo->portResetInfo.state = REGULAR_STATE;
            portInfo->portRebootInfo.rebootState = REBOOT_STATE_IDLE;
            portInfo->portRebootInfo.rebootEvent |= REBOOT_EVENT_IDLE;
            portInfo->portResetInfo.errorCode &= (~ERR_REBOOTING);
            portInfo->portResetInfo.errorCode &= (~ERR_MANUAL_REBOOT);
            if (portInfo->portResetInfo.event & MANUAL_RESTART)
            {
                portInfo->portResetInfo.event &= (~MANUAL_RESTART);
                portInfo->portResetInfo.errorCode &= (~ERR_PORT_SHUTDOWN);
                //getPortParam(portNum - 1, 1);
            }
            break;
    }

    //if (portInfo->portResetInfo.resetCount >= maxReset)
    //{
    //    error_register(ERR_PORT_SHUTDOWN, ERR_LEVEL_CRITICAL,
    //                   "Max reboot attempts", __FILE__, __LINE__);
    //    portInfo->portStateInfo.port_state = PORT_STATE_DISABLED;
    //}
}

static void test_ErrorHandler(portInfo_t *portInfo)
{
    const uint8_t testNum = config_get_value("test_num");
    syslog(LOG_DEBUG, "test_ErrorHandler -- error %x -- testNum = %d", portInfo->portResetInfo.errorCode, testNum);
    if (portInfo->portResetInfo.errorCode & ERR_OK)
    {
        portInfo->portResetInfo.errorTestCount =
                (portInfo->portResetInfo.errorTestCount > 0) ?
                portInfo->portResetInfo.errorTestCount - 1 : 0;
    }
    else if (portInfo->portResetInfo.errorCode & (ERR_TEST_LINK | ERR_TEST_PING | ERR_TEST_SPEED))
    {
        if (++portInfo->portResetInfo.errorTestCount >= testNum)
        {
            if (portInfo->portResetInfo.state != REBOOT_STATE)
            {
                portInfo->portResetInfo.event |= AUTO_RESTART;
                portInfo->portResetInfo.state = REBOOT_STATE;
            }
        }
    }
}

static void alarmHandler(portInfo_t *portInfo)
{
    const time_t now = time(NULL);

    for (int i = 0; i < time_alarm_max; i++)
    {
        time_h_m *alarm = &portInfo->portConfigInfo.timeAlarm[i];

        if (alarm->status && now >= alarm->targetTime) {
            const POE_CONTROL state = (i == time_up) ? POE_UP : POE_DOWN;
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

static void printResult(portInfo_t *portInfo)
{
    static uint8_t testCount = 0;
    static uint8_t errorCode = 0;

    const char *ar_events_str[5] =
    {
            "IDLE_EVENT", "TEST_OK",  "AUTO_RESTART", "---", "MANUAL_RESTART"
    };

    const char *ar_state_str[3] =
    {
            "IDLE_STATE", "REGULAR_STATE", "REBOOT_STATE"
    };

    const char *reboot_state_str[4] =
    {
            "REBOOT_STATE_IDLE", "REBOOT_STATE_INIT", "REBOOT_STATE_POE_DOWN", "REBOOT_STATE_POE_UP"
    };

    const char *reboot_event_str[3] =
    {
            "REBOOT_EVENT_IDLE", "REBOOT_EVENT_START", "REBOOT_EVENT_TIMER_STOP"
    };

    for (int port = 0; port < NUM_PORTS; port++)
    {
        printf("-----------------  portConfigInfo[ %d ] - \n", port + 1);
        printf("portNum     = %d\n", portInfo[port].portConfigInfo.portNum);
        printf("test        = %d\n", portInfo[port].portConfigInfo.testType);
        printf("errorCode   = %x\n", portInfo[port].portResetInfo.errorCode);
        printf("\n-----------------  \n");

        //printf("\t\talarm down        = %d\n", portInfo[port].portConfigInfo.timeAlarm[time_down].status);
        //printf("\t\ttime remain down  = %d\n", portInfo[port].portConfigInfo.timeAlarm[time_down].remainTime);
        //printf("\t\talarm up          = %d\n", portInfo[port].portConfigInfo.timeAlarm[time_up].status);
        //printf("\t\ttime rimain up    = %d\n", portInfo[port].portConfigInfo.timeAlarm[time_up].remainTime);
        //printf("\n-----------------  \n");

        if (portInfo[port].portConfigInfo.testType == TEST_DISABLE)
        {
            printf("test disable -- \n");
        }
        //else if (portInfo[port].portConfigInfo.testType == TEST_SPEED)
        //{
        //    printf("rx_speed_Kbit   = %d\n", portInfo[port].portConfigInfo.rx_speed_Kbit);
        //    printf("rx_delta        = %llu\n", portInfo[port].portConfigInfo.rx_delta_byte);
        //}

        printf("test Count        = %d\n", portInfo->portResetInfo.errorTestCount);
        printf("reset Count       = %d\n", portInfo->portResetInfo.resetCount);
        printf("total Reset Count = %d\n", portInfo->portResetInfo.totalResetCount);
        printf("reset event       = %X -- %s \n", portInfo->portResetInfo.event, ar_events_str[portInfo[port].portResetInfo.event]);
        printf("reset state       = %d -- %s\n", portInfo->portResetInfo.state, ar_state_str[portInfo[port].portResetInfo.state]);
        printf("\n*********************************************\n");
        printf("reboot Event      = %x -- %s\n", portInfo->portRebootInfo.rebootEvent, reboot_event_str[portInfo[port].portRebootInfo.rebootEvent]);
        printf("reboot State      = %d -- %s\n", portInfo->portRebootInfo.rebootState, reboot_state_str[portInfo[port].portRebootInfo.rebootState]);
        printf("reboot TimeStart  = %ld\n", portInfo->portRebootInfo.rebootTimeStart);
        printf("reboot Delay      = %u\n", portInfo->portRebootInfo.rebootDelay);
    }
    //set_notRecivedMsg();
}




