//
// Created by sheverdin on 6/24/24.
//

#include "string.h"
#include "utils_lib/utils_module.h"
#include "client_module.h"

uint8_t isRecivedMsg = 0;
resetPort_U resetPortBuf;

typedef void (*cmdHandler_t) (int fd, uint8_t port);

static void cmd_sendStatus(int fd, uint8_t port);
static void cmd_rebootHandler(int fd, uint8_t port);
static void d_sendCmd(const int *sock_fd, resetPort_U *msg);

cmdHandler_t cmdHandler[max_cmd] =
{
    NULL,
    &cmd_sendStatus,
    &cmd_rebootHandler
};

CLIENT_STATE clientHandler(int fd)
{
    isRecivedMsg = 0;
    sock_msg_t clientMsg;
    CLIENT_STATE clientState = SOCKET_IDLE;
    clientMsg.msg_s.cmdType   = 0,
    clientMsg.msg_s.port      = 0;
    clientState = d_receive_msg(fd, clientMsg.arr);

    if (clientState == SOCKET_READ_OK)
    {
        isRecivedMsg = 1;
        if (cmdHandler[clientMsg.msg_s.cmdType] == NULL)
        {
        }
        else
        {
            cmdHandler[clientMsg.msg_s.cmdType](fd, clientMsg.msg_s.port);
        }
    }
    return clientState;
}

uint8_t *is_ReceivedMsg(void)
{
    return &isRecivedMsg;
}

void set_notRecivedMsg(void)
{
    isRecivedMsg = 0;
}

static void cmd_sendStatus(int fd, uint8_t port)
{
    //portInfo_t *portInfoArr = port_get_portInfoArr();
    //for (int portNum = 0; portNum < NUM_PORTS; portNum++)
    //{
    //   resetPortBuf.status.resetPort[portNum].errorCode     = portInfoArr[portNum].portResetInfo.errorCode;
    //   resetPortBuf.status.resetPort[portNum].portNum       = portInfoArr[portNum].portConfigInfo.portNum;
    //   resetPortBuf.status.resetPort[portNum].resetCount    = portInfoArr[portNum].portResetInfo.totalResetCount;
    //   resetPortBuf.status.resetPort[portNum].rx_speed_Kbit = portInfoArr[portNum].portConfigInfo.rx_speed_Kbit;
    //   resetPortBuf.status.resetPort[portNum].testType      = portInfoArr[portNum].portConfigInfo.test;
    //   resetPortBuf.status.resetPort[portNum].status        = 0;
    //   strcpy(resetPortBuf.status.resetPort[portNum].timeStr, "");
    //   strcat(resetPortBuf.status.resetPort[portNum].timeStr, portInfoArr[portNum].portConfigInfo.timeStr);
    //   strcat(resetPortBuf.status.resetPort[portNum].timeStr, "\0");
    //}
    //d_sendCmd(&fd,  &resetPortBuf);
}

static void cmd_rebootHandler(int fd, uint8_t port)
{
    //portInfo_t *portInfoArr = port_get_portInfoArr();
    //resetPortBuf.status.resetPort[port-1].status = 0;
    //
    //if (portInfoArr[port - 1].portResetInfo.state != REBOOT_STATE)
    //{
    //    if (portInfoArr[port - 1].portStateInfo.poe_state == BOOL_ENABLED
    //        && portInfoArr[port - 1].portStateInfo.port_state == BOOL_ENABLED)
    //    {
    //        portInfoArr[port - 1].portResetInfo.state = REBOOT_STATE;
    //        portInfoArr[port - 1].portResetInfo.event |= MANUAL_RESTART;
    //        openlog("tf_autorestart", 0, LOG_USER);
    //        syslog(LOG_ALERT, "port <%d>: Manual reboot .... ", port);
    //        closelog();
    //        resetPortBuf.status.resetPort[port - 1].status = 1;
    //    }
    //    else
    //    {
    //        resetPortBuf.status.resetPort[port - 1].status = 3;
    //    }
    //}
    //else
    //{
    //    portInfoArr[port - 1].portResetInfo.event &= (~MANUAL_RESTART);
    //    openlog("tf_ar_poe", 0, LOG_USER);
    //    syslog(LOG_ALERT, "port <%d>: Manual reboot disable, port in rebooting sate", port);
    //    closelog();
    //    resetPortBuf.status.resetPort[port-1].status = 2;
    //}
    //resetPortBuf.status.resetPort[port - 1].errorCode  = portInfoArr[port - 1].portResetInfo.errorCode;
    //resetPortBuf.status.resetPort[port - 1].portNum    = portInfoArr[port - 1].portConfigInfo.portNum;
    //resetPortBuf.status.resetPort[port - 1].resetCount = portInfoArr[port - 1].portResetInfo.totalResetCount;
    //resetPortBuf.status.resetPort[port - 1].testType   = portInfoArr[port - 1].portConfigInfo.test;
    //strcpy(resetPortBuf.status.resetPort[port - 1].timeStr, "");
    //strcat(resetPortBuf.status.resetPort[port - 1].timeStr, portInfoArr[port - 1].portConfigInfo.timeStr);
    //strcat(resetPortBuf.status.resetPort[port - 1].timeStr, "\0");
    //d_sendCmd(&fd,  &resetPortBuf);
}

static void d_sendCmd(const int *sock_fd, resetPort_U *msg)
{
    d_send_msg(*sock_fd,  msg->arr, sizeof(msg->arr));
}