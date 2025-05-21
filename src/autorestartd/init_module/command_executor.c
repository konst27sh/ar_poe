//
// Created by sheverdin on 5/15/25.
//

#include "command_executor.h"
#include <syslog.h>
#include <string.h>

json_t* execute_ubus_command(const char* command_template, const char* arg) {
    char cmd[MAX_CMD_LEN];
    char response[MAX_RESPONSE] = {0};
    FILE* pipe = NULL;

    if (snprintf(cmd, sizeof(cmd), command_template, arg) < 0) {
        syslog(LOG_ERR, "Command formatting failed");
        return NULL;
    }

    pipe = popen(cmd, "r");
    if (!pipe) {
        syslog(LOG_ERR, "Failed to execute command: %s", cmd);
        return NULL;
    }

    char chunk[128];
    while (fgets(chunk, sizeof(chunk), pipe))
    {
        strncat(response, chunk, sizeof(response) - strlen(response) - 1);
    }
    pclose(pipe);

    json_error_t error;
    json_t* root = json_loads(response, 0, &error);
    syslog(LOG_DEBUG, "cmd = %s", cmd);
    syslog(LOG_DEBUG, "response = %s", response);

    if (root == NULL)
    {
        syslog(LOG_ERR, "JSON parse error 1: %s (line %d)", error.text, error.line);
        return NULL;
    }

    return root;
}



