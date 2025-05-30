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

#include "port_manager.h"
#include "init_module/error_handler.h"
#include "init_module/command_executor.h"
#include "init_module/error_handler.h"

static void runTestHandler(void);
static void* runComHandler(void *args);
static void runEventsHandler(void);

#define THREADS_NUM (3)

//mainThreads_t ar_mainThreads[] =
//{
//    &runTestHandler,
//    &runComHandler,
//    &runEventsHandler
//};

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
//        error_register((error_code_t) ERR_OK, ERR_LEVEL_DEBUG,
//                       "Debug mode enabled", __FILE__, __LINE__);
//    }
//
//
//    for (uint8_t i = 0; i < NUM_PORTS; i++)
//    {
//        port_config_info_t port_cfg;
//        if (port_manager_get_config(i, &port_cfg))
//        {
//            error_register(ERR_UNAVAILABLE_RESOURCE, ERR_LEVEL_WARNING,
//                           "Invalid port portConfigInfo", __FILE__, __LINE__);
//        }
//    }
//
//}

void ar_mainApp(void)
{
    //uint8_t p_err = createThread(THREADS_NUM, ar_mainThreads);
    //if (p_err != 0)
    //{
    //    error_register(ERR_CREATE_THREAD, ERR_LEVEL_CRITICAL,
    //                   "Thread creation failed", __FILE__, __LINE__);
    //    return;
    //}
    //p_err = joinThread(THREADS_NUM);
    //if (p_err != 0)
    //{
    //    error_register(ERR_CREATE_THREAD, ERR_LEVEL_CRITICAL,
    //                   "Thread joinThread", __FILE__, __LINE__);
    //}
    runTestHandler();

}

static void runTestHandler(void)
{
    int test_delay = config_get_value("test_delay");

    while (keepRunning)
    {
        syslog(LOG_ALERT, "=============== keepRunning ==================");
        for (uint8_t port = 0; port < NUM_PORTS; port++)
        {
            syslog(LOG_ALERT, ">>>>>>>>>>> Port %d >>>>>>>>>>>>>>>>>>>>>>>>>>> ", port+1);
            port_config_info_t cfg;
            if (port_manager_get_config(port, &cfg) != 0)
            {
                syslog(LOG_DEBUG, "port_manager_get_config != 0 == Port %d: test type %d", port + 1, cfg.testType);
                continue;
            }

            AR_STATE ar_state = port_manager_get_ar_state(port);
            if (ar_state != REGULAR_STATE)
            {
                syslog(LOG_DEBUG, "ar_state != REGULAR_STATE");
                continue;
            }

            switch (cfg.testType)
            {
                case TEST_LINK:
                    port_run_test_link(port);
                    break;
                case TEST_PING:
                   port_run_test_link(port);
                   port_run_test_ping(port);
                    break;
                case TEST_SPEED:
                    port_run_test_link(port);
                    port_run_test_speed(port);
                    break;
                case TEST_DISABLE:
                    port_run_test_disable(port);
                    break;
                case TEST_MAX:
                    break;
            }
        }
        eventsHandler();
        sleep(test_delay);
    }
    //return NULL;
}

static void* runComHandler(void *args)
{
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
        //if (accept_socket(sock_fd, &client_fd) == SOCKET_ERR_OK)
        //{
        //
        //    syslog(LOG_ALERT, "accept_socket = %d -- %d", client_fd, client_fd % NUM_PORTS);
        //}
         sleep(test_delay*5);
    }
    return NULL;
}

static void runEventsHandler(void)
{
    int test_delay = config_get_value("test_delay");
    while (keepRunning)
    {
        eventsHandler();
        sleep(test_delay);
    }
   //return NULL;
}


