cmake_minimum_required(VERSION 3.25)
project(tf_autorestart C)
set(CMAKE_C_STANDARD 17)

include(CheckIncludeFile)
check_include_file(execinfo.h HAVE_EXECINFO_H)
if(HAVE_EXECINFO_H)
    add_compile_definitions(HAVE_EXECINFO_H)
endif()

# Добавление -rdynamic только если доступен backtrace
if(HAVE_EXECINFO_H)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -rdynamic")
endif()

add_library(socket_module STATIC
        lib/socket_lib/socket_module.c
        lib/socket_lib/socket_module.h
)

add_library(thread_module STATIC
        lib/thread_lib/thread_module.c
        lib/thread_lib/thread_module.h
)

add_library(utils_module STATIC
        lib/utils_lib/utils_module.c
        lib/utils_lib/utils_module.h
)

# Add the include directories
target_include_directories(socket_module PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_include_directories(thread_module  PUBLIC ${PROJECT_SOURCE_DIR}/lib)
target_include_directories(utils_module  PUBLIC ${PROJECT_SOURCE_DIR}/lib)

# Create the project_2 executable
add_executable(tf_autorestartd
        src/autorestartd/ar_maind.c
        src/autorestartd/ar_maind.h
        src/autorestartd/ar_mainApp.c
        src/autorestartd/ar_mainApp.h
        src/autorestartd/client_module.c
        src/autorestartd/client_module.h
        src/autorestartd/events_module.c
        src/autorestartd/events_module.h
        src/autorestartd/init_module.c
        src/autorestartd/init_module.h
        src/autorestartd/init_module/port_manager.c
        src/autorestartd/init_module/config_loader.c
        src/autorestartd/init_module/command_executor.c
        src/autorestartd/init_module/error_handler.c
        src/autorestartd/init_module/config_loader.h
        src/autorestartd/init_module/port_manager.h
        src/autorestartd/init_module/command_executor.h
        src/autorestartd/init_module/error_handler.h
        src/autorestartd/app_control.h
)

target_link_libraries(tf_autorestartd  socket_module thread_module utils_module jansson)
target_link_options(tf_autorestartd PRIVATE -rdynamic)
INSTALL(TARGETS tf_autorestartd RUNTIME DESTINATION /usr/bin/)

add_executable(tf_autorestart_ctrl
        src/autorestart_ctrl/ar_main_ctrl.c
        src/autorestart_ctrl/parsing_command.c
        src/autorestart_ctrl/parsing_command.h
        src/autorestart_ctrl/client_module.c
        src/autorestart_ctrl/client_module.h
)

target_link_libraries(tf_autorestart_ctrl PRIVATE socket_module thread_module utils_module jansson)
target_link_options(tf_autorestart_ctrl PRIVATE -rdynamic)


INSTALL(TARGETS tf_autorestart_ctrl RUNTIME DESTINATION /usr/bin/)