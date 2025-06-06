//
// Created by sheverdin on 12/28/23.
//

#include "parsing_command.h"

const char cmdTable[max_cmd][COMMAND_LENGTH] =
{
    {" "},
    {"status"},
    {"reboot"}
};

uint8_t getCmdType(char *cmd, CMD_TYPE_e *cmdType)
{
    uint8_t errorCode = 1;
    *cmdType = cmd_idle;
    for (CMD_TYPE_e i = 0; i < max_cmd; i++)
    {
        if (strcmp(cmd, cmdTable[i]) == 0)
        {
            *cmdType = i;
            i = max_cmd;
            errorCode = 0;
        }
    }
    return errorCode;
}

void getPortNum(char *portStr, uint8_t *portNum)
{
    uint8_t intValue  = 0;
    intValue = (uint8_t)(strtol(portStr, NULL, 10));
    if (intValue < 1 || intValue > 8)
    {
        *portNum    = 0;
    } else {
        *portNum = intValue;
    }
}

void printBufferJson(resetPort_U *resetStatic)
{
    size_t lenErrorMsg      = 0;
    size_t len_programmMsg  = 0;
    char programmMsg[256];
    strcpy(programmMsg, "");
    if(resetStatic->status.arCtrlError & CTRL_ERROR_OK) {
        strcpy(programmMsg, "ok");
    }
    if(resetStatic->status.arCtrlError & CTRL_ERROR_DEBUG) {
        len_programmMsg = strlen(programmMsg);
        if (len_programmMsg)
            strcat(programmMsg, ", ");
        strcat(programmMsg, "debug  mode");
    }
    if(resetStatic->status.arCtrlError & CTRL_ERROR_ARG_1) {
        len_programmMsg = strlen(programmMsg);
        if (len_programmMsg)
            strcat(programmMsg, ", ");
        strcat(programmMsg, "usage: stataus or reboot <port>");
    }
    if(resetStatic->status.arCtrlError & CTRL_ERROR_ARG_2) {
        len_programmMsg = strlen(programmMsg);
        if (len_programmMsg)
            strcat(programmMsg, ", ");
        strcat(programmMsg, "error: port num not coorrected");
    }
    if(resetStatic->status.arCtrlError & CTRL_ERROR_ARG_3) {
        len_programmMsg = strlen(programmMsg);
        if (len_programmMsg)
            strcat(programmMsg, ", ");
        strcat(programmMsg, "eror: usage reboot <port>");
    }
    if(resetStatic->status.arCtrlError & CTRL_ERROR_SOCKET) {
        len_programmMsg = strlen(programmMsg);
        if (len_programmMsg)
            strcat(programmMsg, ", ");
        strcat(programmMsg, "error: socket");
    }
    if(resetStatic->status.arCtrlError & CTRL_ERROR_INACTIVE) {
        len_programmMsg = strlen(programmMsg);
        if (len_programmMsg)
            strcat(programmMsg, ", ");
        strcat(programmMsg, "error: demon not running");
    }

    printf("{\n");

    printf("\"programm_error\":\"0x%x\",\n", resetStatic->status.arCtrlError);
    printf("\"programm_msg\":\"%s\",\n", programmMsg);

    printf("\"port\":[\n ");

    for (int portNum = 0; portNum < NUM_PORTS; portNum++)
    {
        char errorMsg[256];
        strcpy(errorMsg, "");
        if(resetStatic->status.resetPort[portNum].errorCode == ERR_OK) {
            resetStatic->status.resetPort[portNum].errorCode = 0;
            strcpy(errorMsg, "OK");
        }
        if(resetStatic->status.resetPort[portNum].errorCode != ERR_OK)
        {
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_MANUAL_REBOOT) {
                strcat(errorMsg, "manual rebooting... ");
            }
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_REBOOTING) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "auto rebooting... ");
            }
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_POE_DOWN) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "info: PoE switched off according to the schedule");
            }
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_UNAVAILABLE_RESOURCE) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "error: unavailable resource");
            }
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_NULL_OBJ) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "error: obj null");
            }
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_CREATE_THREAD) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "error: create thread");
            }
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_JOIN_THREAD) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "error: join thread");
            }
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_PoE_DISABLE) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "error: PoE disabled");
            }
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_TEST_TYPE) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "error: test type");
            }
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_IP_NOT_VALID) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "error: ip not valid");
            }
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_TIME_NOT_VALID) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "error: time not valid");
            }
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_SPEED_VALUE) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "error: speed not in range");
            }
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_PORT_DISABLE) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "port disable");
            }
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_TEST_DISABLE) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "test disable");
            }
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_TEST_LINK) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "error: test link");
            }
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_TEST_PING) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "error: test ping");
            }
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_TEST_SPEED) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "error: test speed");
            }
            if(resetStatic->status.resetPort[portNum].errorCode & ERR_TIME_ALARM) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "error: time alarm");
            }

            if(resetStatic->status.resetPort[portNum].errorCode & ERR_PORT_SHUTDOWN) {
                lenErrorMsg = strlen(errorMsg);
                if (lenErrorMsg)
                    strcat(errorMsg, ", ");
                strcat(errorMsg, "error: reboot disabled after max resets");
            }
        }
        if (portNum < (NUM_PORTS-1))
        {
            printf("{ \n"
                   "\t\"id\":\"%d\",\n"
                   "\t\"error_Code\":\"%d\",\n"
                   "\t\"status\":\"%s\",\n"
                   "\t\"reboot_cnt\":\"%d\",\n"
                   "\t\"time\":\"%s\",\n"
                   "\t\"test_type\":\"%d\",\n"
                   "\t\"speed_kbit\":\"%d\",\n"
                   "\t\"manual_reboot\":\"%d\""
                   "\n },"
                   "\n",
                   resetStatic->status.resetPort[portNum].portNum,     resetStatic->status.resetPort[portNum].errorCode, errorMsg,
                   resetStatic->status.resetPort[portNum].resetCount,  resetStatic->status.resetPort[portNum].timeStr,
                   resetStatic->status.resetPort[portNum].testType,    resetStatic->status.resetPort[portNum].rx_speed_Kbit,
                   resetStatic->status.resetPort[portNum].status);
        }
        else
        {
            printf("{ \n"
                   "\t\"id\":\"%d\",\n"
                   "\t\"error_Code\":\"%d\",\n"
                   "\t\"status\":\"%s\",\n"
                   "\t\"reboot_cnt\":\"%d\",\n"
                   "\t\"time\":\"%s\",\n"
                   "\t\"test_type\":\"%d\",\n"
                   "\t\"speed_kbit\":\"%d\",\n"
                   "\t\"manual_reboot\":\"%d\""
                   "\n }"
                   "\n",
                   resetStatic->status.resetPort[portNum].portNum,     resetStatic->status.resetPort[portNum].errorCode, errorMsg,
                   resetStatic->status.resetPort[portNum].resetCount,  resetStatic->status.resetPort[portNum].timeStr,
                   resetStatic->status.resetPort[portNum].testType,    resetStatic->status.resetPort[portNum].rx_speed_Kbit,
                   resetStatic->status.resetPort[portNum].status);
        }
    }
    printf("]\n }\n");
}




