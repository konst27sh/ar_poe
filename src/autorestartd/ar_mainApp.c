//
// Created by sheverdin on 6/24/24.
//

#include "ar_mainApp.h"
#include "thread_lib/thread_module.h"
#include "utils_lib/utils_module.h"

#include "client_module.h"
#include "events_module.h"
#include "init_module.h"
#include "app_control.h"

#include "init_module/port_manager.h"
#include "init_module/error_handler.h"
#include "init_module/command_executor.h"
#include "init_module/error_handler.h"

static void* runTestHandler(void *args);
static void* runComHandler(void *args);
static void* runEventsHandler(void *args);

#define THREADS_NUM (3)

mainThreads_t ar_mainThreads[] =
{
    &runTestHandler,
    &runComHandler,
    &runEventsHandler
};

//void ar_mainInit(int argc, char **argv)
//{
//    if (argc == 2 && argv[1][0] == '-')
//    {
//        setTestMode(argv[1][1]);
//    }
//
//    uint8_t *isDebug = isDebugMode();
//    if (*isDebugMode())
//    {
//        printf("tf_ar_poe VERSION: %s\n", VERSION);
//        error_register((error_code_t) ERR_OK, ERR_LEVEL_DEBUG,
//                       "Debug mode enabled", __FILE__, __LINE__);
//    }
//
//    //update_config("ar_demon");
//
//    for (uint8_t i = 0; i < NUM_PORTS; i++)
//    {
//        port_config_info_t port_cfg;
//        if (port_manager_get_config(i, &port_cfg))
//        {
//            error_register(ERR_UNAVAILABLE_RESOURCE, ERR_LEVEL_WARNING,
//                           "Invalid port config", __FILE__, __LINE__);
//        }
//    }
//
//    //getPortsParam();
//    //rebootParamInit();
//    runTimer();
//}

void ar_mainApp(void)
{
    uint8_t p_err = createThread(THREADS_NUM, ar_mainThreads);
    printf("ar_mainApp - p_err = %d\n", p_err);
    if (p_err != 0)
    {
        error_register(ERR_CREATE_THREAD, ERR_LEVEL_CRITICAL,
                       "Thread creation failed", __FILE__, __LINE__);
        return;
    }
    printf("ar_mainApp - joinThread\n");
    p_err = joinThread(THREADS_NUM);
    if (p_err != 0)
    {
        printf("ERROR Join thread\n");
    }
}

static void* runTestHandler(void *args)
{
    syslog(LOG_INFO, "runTestHandler >>>>>");
    printf("runTestHandler >>>>> \n");
    int test_delay = config_get_value("test_delay");
    int max_reset = config_get_value("max_reset");
    syslog(LOG_INFO, "test_delay = %d", test_delay);

    while (keepRunning)
    {
        for (uint8_t port = 0; port < NUM_PORTS; port++)
        {
            PortConfig cfg;
            if (port_manager_get_config(port, &cfg) != 0) continue;

            syslog(LOG_DEBUG, "Port %d Config: test=%s, host=%s, min=%u, max=%u",
                   port + 1,
                   test_type_to_str(cfg.testType),
                   cfg.host,
                   cfg.minSpeed,
                   cfg.maxSpeed);

            AR_STATE ar_state = port_manager_get_ar_state(port);
            if (ar_state != REGULAR_STATE)
            {
                syslog(LOG_DEBUG, "Port %d: AR State=%d", port + 1, ar_state);
                continue;
            }

            error_code_t err = ERR_OK;
            switch (cfg.testType) {
                case TEST_LINK:
                    err = port_run_test_link(port);
                    break;
                case TEST_PING:
                    err = port_run_test_ping(port);
                    break;
                case TEST_SPEED:
                    err = port_run_test_speed(port);
                    break;
            }

            // Обработка результатов
            if (err != ERR_OK)
            {
                // port_manager_auto_reset(port, err, cfg.maxReset);  // TODO
                port_manager_auto_reset(port, err, max_reset);
                syslog(LOG_INFO, "Port %d: Test failed (Error: 0x%X)", port + 1, err);
            }
        }
        sleep(test_delay);
    }
    return NULL;
}

static void* runComHandler(void *args)
{
    syslog(LOG_INFO, "runComHandler >>>>>");
    printf("runComHandler >>>>> \n");

    int sock_fd;
    //if (socket_asyncServer(&sock_fd) != SOCKET_ERR_OK)
    //{
    //    error_register(ERR_SOCKET, ERR_LEVEL_CRITICAL,
    //                   "Socket creation failed", __FILE__, __LINE__);
    //    return NULL;
    //}
    int test_delay = config_get_value("test_delay");
    while (keepRunning)
    {
        int client_fd;
        printf("wait for socket\n");
        //if (accept_socket(sock_fd, &client_fd) == SOCKET_ERR_OK)
        //{
        //    printf("socket accepted\n");
        //    syslog(LOG_ALERT, "accept_socket = %d -- %d", client_fd, client_fd % NUM_PORTS);
        //    handle_port_command(client_fd % NUM_PORTS, CMD_GET_PORT_STATUS);
        //}
         sleep(test_delay*5);
    }
    return NULL;
}

static void* runEventsHandler(void *args)
{
    syslog(LOG_INFO, "runEventsHandler >>>>>");
    printf("runEventsHandler >>>>> \n");

    int test_delay = config_get_value("test_delay");
    printf("configOptions[TEST_DELAY].value = %d\n", test_delay);
    //for (int i = 0; i < MAX_PARM; i++)
    //{
    //    getConfigOptions(&configOptions[i], i);
    //}
    while (keepRunning)
    {
        //eventsHandler();
        printf("runEventsHandler loop = ");
        sleep(test_delay*5);
    }
   return NULL;
}


