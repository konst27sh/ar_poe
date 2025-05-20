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

int handle_port_command(uint8_t port_num, CommandType cmd_type)
{

    printf("handle_port_command -- port %d --- cmdType = %d", port_num, cmd_type);
    if (port_num >= NUM_PORTS) {
        syslog(LOG_ERR, "Invalid port number: %d", port_num);
        return -1;
    }

    PortConfig config;
    if (port_manager_get_config(port_num, &config) != 0)
    {
        return -1;
    }

    switch (cmd_type)
    {
        case CMD_REBOOT_PORT:
            syslog(LOG_INFO, "Rebooting port %d", port_num + 1);
            return port_manager_set_poe(port_num, POE_STATE_OFF) ||
                   port_manager_set_poe(port_num, POE_STATE_ON);

        case CMD_GET_PORT_STATUS: {
            PortStatus status;
            printf( "CMD_GET_PORT_STATUS -- port %d\n", port_num + 1);
            if (port_manager_get_status(port_num, &status) != 0)
            {
                return -1;
            }

            json_t* response = json_object();
            json_object_set_new(response, "port", json_integer(port_num + 1));
            json_object_set_new(response, "poe_state",
                                json_string(status.poeState == POE_STATE_ON ? "on" : "off"));

            // ... другие поля

            json_dump_file(response, "/tmp/port_status.json", 0);
            json_decref(response);
            break;
        }

        default:
            syslog(LOG_WARNING, "Unknown command type: %d", cmd_type);
            return -1;
    }
    return 0;
}

void update_config(const char* section_name)
{
    json_t* root = execute_ubus_command(
            "ubus call uci get '{\"config\":\"tf_autorestart\", \"section\":\"%s:w\"}'",section_name);
    if (root != NULL)
    {
        config_load_section("tf_autorestart", section_name);
        json_decref(root);
    }
}