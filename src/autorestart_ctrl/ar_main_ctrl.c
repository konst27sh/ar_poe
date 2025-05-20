//
// Created by sheverdin on 22/12/23.
//
#include "../global_includs.h"
#include "parsing_command.h"
#include "client_module.h"
#include "utils_lib/utils_module.h"

AR_CTRL_ERROR flag_isRunning = CTRL_ERROR_OK;
resetPort_U resetPort;

static AR_CTRL_ERROR isRunning(void);
static AR_CTRL_ERROR ctrl_mainHandler(int argc, char **argv);
static SOCKET_ERR_t comHandler(CMD_TYPE_e cmdTypeE, uint8_t port, unsigned char *resetMsg);

int main(int argc, char **argv)
{
    flag_isRunning = CTRL_ERROR_OK;

    if (argc < 2 ){
        flag_isRunning = CTRL_ERROR_ARG_1;
        //printf("Usage: \n");
        //printf("\t-status\n");
        //printf("\t-reboot <port>\n");
    }
    else
    {
        flag_isRunning = isRunning();
        if (flag_isRunning & CTRL_ERROR_OK)
        {
            flag_isRunning |= ctrl_mainHandler(argc, argv);
        }
        else
        {
            resetPort.status.arCtrlError = flag_isRunning;
            printBufferJson(&resetPort);
        }
    }
    return flag_isRunning;
}

static AR_CTRL_ERROR ctrl_mainHandler(int argc, char **argv)
{
    AR_CTRL_ERROR errorCode = CTRL_ERROR_OK;
    SOCKET_ERR_t socketErr = SOCKET_ERR_OK;
    CMD_TYPE_e cmdType  = cmd_idle;
    uint8_t portNum     = 0;
    getCmdType(argv[1], &cmdType);
    portNum = 0;

    if(cmdType == cmd_idle)
    {
        flag_isRunning = CTRL_ERROR_ARG_1;
        //printf("command type is wrong\n");
    }
    else if (cmdType == cmd_status)
    {
        socketErr = comHandler(cmdType, 0, resetPort.arr);
    }
    else if (cmdType == cmd_reboot)
    {
        if (argc == 3)
        {
            getPortNum( argv[2], &portNum);
            if (portNum)
            {
                socketErr = comHandler(cmdType, portNum, resetPort.arr);
            }
            else
            {
                errorCode = CTRL_ERROR_ARG_2;
                openlog("tf_autoresart_ctrl", 0, LOG_USER);
                syslog(LOG_ERR, "port <%d>: error port number not corrected", portNum);
                closelog();
            }
        }
        else
        {
            errorCode = CTRL_ERROR_ARG_3;
            //printf("for Reboot needs <port> - value in range 1-8\n");
        }
    }

    if (socketErr != SOCKET_ERR_OK)
    {
        flag_isRunning |= CTRL_ERROR_SOCKET;
    }

    resetPort.status.arCtrlError = flag_isRunning;
    printBufferJson(&resetPort);
    return errorCode;
}

static SOCKET_ERR_t comHandler(CMD_TYPE_e cmdTypeE, uint8_t port, unsigned char *resetMsg)
{
    int sock_fd;

    SOCKET_ERR_t socketErr = ar_ctrl_create_socket(&sock_fd);
    sock_msg_t msg =
    {
        .msg_s.cmdType  = cmdTypeE,
        .msg_s.port     = port,
    };

    if (socketErr == SOCKET_ERR_OK)
    {
        ctrl_sendCmd(&sock_fd, &msg);
        CLIENT_STATE clientState = ctrl_getresult(&sock_fd, resetMsg);

        if (clientState != SOCKET_READ_OK)
        {
            socketErr = SOCKET_ERR_READ;
        }
    }
    return socketErr;
}

static AR_CTRL_ERROR isRunning(void)
{
    char res[32];
    strcpy(res, "");

    char output[OUTPUT_MAX_LENGTH];
    strcpy(output, "");
    FILE *pipe_isRunning = NULL;
    char cmd_getProcessList[] = "/usr/share/lua/getProcessList.lua";
    char process_name[] = " /usr/bin/tf_autorestartd";
    if(utils_check_file_exists(cmd_getProcessList))
    {
        char cmd_isRunning[60];
        strcpy(cmd_isRunning, "");
        strncpy(cmd_isRunning, cmd_getProcessList, sizeof(cmd_getProcessList));
        strcat(cmd_isRunning, process_name);
        pipe_isRunning = openPipe(cmd_isRunning);

        while (fgets(output, OUTPUT_MAX_LENGTH, pipe_isRunning) != NULL)
        {
        }
        closePipe(pipe_isRunning, cmd_isRunning);
        if (strcmp(output, "running\n") == 0)
        {
            //printf("running\n");
            flag_isRunning = CTRL_ERROR_OK;
        }
        else
        {
            //printf("inactive\n");
            flag_isRunning |= CTRL_ERROR_INACTIVE;
            flag_isRunning &= (~CTRL_ERROR_OK);
        }
    }

    //printf("arCtrlError = %d\n", flag_isRunning);
    return flag_isRunning;
}


