//
// Created by sheverdin on 5/16/25.
//

#include "init_module_tests.h"

#include "../autorestartd/init_module/config_loader.h"
#include "../autorestartd/init_module/port_manager.h"
#include <jansson.h>
#include <stdio.h>
#include <assert.h>

void test_config_load_valid() {
    // Подготовка тестового конфига
    const char *test_config =
            "{ \"values\": { "
            "   \"test_delay\": \"5\", "
            "   \"max_reset\": \"3\" "
            "}}";

    // Загрузка конфигурации
    load_config_from_string(test_config);

    // Проверка значений
    assert(config_get_value("test_delay") == 5);
    assert(config_get_value("max_reset") == 3);

    printf("[OK] test_config_load_valid\n");
}

void test_config_load_invalid_values() {
    const char *test_config =
            "{ \"values\": { "
            "   \"test_delay\": \"100\", "  // > max_val (60)
            "   \"max_reset\": \"abc\" "    // не число
            "}}";

    load_config_from_string(test_config);

    // Должны быть установлены значения по умолчанию
    assert(config_get_value("test_delay") == 3); // default = 3
    assert(config_get_value("max_reset") == 3);  // default = 3

    printf("[OK] test_config_load_invalid_values\n");
}

void test_port_init_defaults() {
    port_manager_init();

    port_config_info_t config;
    port_manager_get_config(0, &config);

    // Проверка параметров по умолчанию
    assert(config.testType == ERR_TEST_DISABLE);
    assert(strcmp(config.host, "0.0.0.0") == 0);

    printf("[OK] test_port_init_defaults\n");
}

void test_port_load_params() {
    // Тестовая конфигурация порта
    const char *test_config =
            "{ \"values\": { "
            "   \"mode\": \"ping\", "
            "   \"host\": \"192.168.1.1\" "
            "}}";

    // Имитация UBUS-запроса
    json_t *root = json_loads(test_config, 0, NULL);
    //process_port_config(0, root); // 0 = первый порт

    // Проверка
    port_config_info_t config;
    port_manager_get_config(0, &config);
    assert(config.testType == ERR_TEST_PING);
    assert(strcmp(config.host, "192.168.1.1") == 0);

    printf("[OK] test_port_load_params\n");
}

void test_full_init_flow() {
    // 1. Загрузка конфигурации
    test_config_load_valid();

    // 2. Инициализация портов
    port_manager_init();

    // 3. Проверка состояния
    PortStatus status;
    port_manager_get_status(0, &status);
    assert(status.poeState == POE_STATE_ON);

    printf("[OK] test_full_init_flow\n");
}

int main() {
    test_config_load_valid();
    test_config_load_invalid_values();
    test_port_init_defaults();
    //test_port_load_params();
    //test_reboot_counters_reset();
    //test_reboot_counter_increment();
    test_full_init_flow();
    return 0;
}
