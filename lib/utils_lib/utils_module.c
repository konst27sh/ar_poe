//
// Created by sheverdin on 6/24/24.
//

// version 1

#include "utils_module.h"
#include <time.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "syslog.h"
#include <regex.h>
#include <jansson.h>
#include <sys/stat.h>

typedef enum
{
    ERR_OK              = 0,
    ERR_PoE_DISABLE     = 0x02000,
    ERR_IP_NOT_VALID    = 0x01000,
    ERR_TIME_NOT_VALID  = 0x04000
}utils_error_code_t;

uint8_t globalDebugMode = 0;

void setTestMode(char testMode)
{
    globalDebugMode = 0;
    if (testMode == 'd'){
        globalDebugMode = 1;
    }
}

void printf_array(uint8_t *buff,uint32_t size){
    char temp[16];
    char printf_buff[128];
    //печатаем по 16 символов в строке
    for(uint8_t i=0; i < (size/16); i++){
        printf_buff[0] = 0;
        for(uint8_t j=0;j<16;j++){
            sprintf(temp,"%02X ",buff[i*16+j]);
            strcat(printf_buff,temp);
        }
        printf("%s\r\n",printf_buff);
    }
    //для остатка
    printf_buff[0] = 0;
    for(int i=0; i < size%16; i++){
        sprintf(temp,"%02X ",buff[(size/16)*16+i]);
        strcat(printf_buff,temp);
    }
    printf("%s\r\n",printf_buff);
}

uint8_t* isDebugMode(void)
{
    return &globalDebugMode;
}

void timer_set(struct timer *t, clock_time_t interval)
{
    t->interval = interval;
    t->start = time(NULL);
}

void timer_reset(struct timer *t)
{
    t->start += t->interval;
}

int timer_expired(struct timer *t)
{
    return (clock_time_t)(time(NULL) - t->start) >= (clock_time_t)t->interval;
}

int getValueFromJson(char *json, char *name, char *value)
{
    char *ptr = strstr(json, name);
    if (ptr == NULL) {
        //printf("Error: %s not found in JSON\n", name);
        value = NULL;
        return  0;
    }
    ptr = strchr(ptr, ':');
    ptr = strchr(ptr, '"');
    sscanf(ptr, "\"%16[^\"]\"", value);
    return  1;
}

int getData_formJson(char* json_data, char *option, char *data)
{
    json_t *root;
    json_error_t error;
    const char *value = NULL;
    root = json_loads(json_data, 0, &error);
    if (!root) {
        fprintf(stderr, "error: on line %d: %s\n", error.line, error.text);
        return 1;
    }
    json_t *values = json_object_get(root, "values");
    if (values == NULL)
    {
        return 2;
    }
    json_t *host = json_object_get(values, option);
    if (host == NULL)
    {
        return 4;
    }

    value = json_string_value(host);
    strcpy(data, value);
    json_decref(root);
    return 8;
}

FILE *openPipe(const char *cmdStr)
{
    FILE *pipe = popen(cmdStr, "r");
    if (!pipe) {
        openlog("ar_err", LOG_PERROR, LOG_USER);
        syslog(LOG_ERR, "error: command = %s", cmdStr);
        closelog();
    }
    return pipe;
}

void closePipe(FILE *pipe, char *str)
{
    int status = pclose(pipe);

    if (!status) {
        return;
    } else{
        openlog("ar_err",  LOG_PERROR, LOG_USER);
        syslog(LOG_ERR, "error close pipe -> %s", str);
        closelog();
    }
}

#define WORD_LENGTH         (10)
const char monthTable[13][WORD_LENGTH] =
{
    {"\n"},             //  0
    {"January\0"},      //  1
    {"February\0"},     //  2
    {"March\0"},        //  3
    {"April\0"},        //  4
    {"May\0"},          //  5
    {"June\0"},         //  6
    {"Julie\0"},        //  7
    {"August\0"},       //  8
    {"September\0"},    //  9
    {"October\0"},      //  10
    {"November\0"},     //  11
    {"December\0"},     //  12
};

time_error_t getTimeDate(char *timeDateStr, size_t bufferSize)
{
    const time_t rawtime = time(NULL);
    if (rawtime == (time_t)-1)
    {
        snprintf(timeDateStr, bufferSize, "N/A");
        return TIME_ERR_SYSYTEM;
    }
    struct tm timeinfo;
    if (!localtime_r(&rawtime, &timeinfo))
    {
        snprintf(timeDateStr, bufferSize, "N/A");
        return TIME_ERR_CONVERT;
    }

    // Форматирование: "HH:MM:SS, Mmm-YYYY" (пример: "14:05:23, Dec-2023")
    const size_t written = strftime(timeDateStr, bufferSize, "%T, %b-%Y", &timeinfo);

    if (written == 0)
    {
        snprintf(timeDateStr, bufferSize, "N/A");
        return TIME_ERR_BUFFER;
    }
    return TIME_ERR_OK;
}

char decimalNum[9][2] = {"0", "1", "2", "3", "4", "5", "6", "7", "8"};
void toString(int num, char *str)
{
    //str = decimalNum[num];
    strcpy(str, decimalNum[num]);
}

void checkValidIp(char ipStr[], int *ipIsValid)
{
    char ip[16];
    int  flag = 0;
    char pattern[] = "^([0-9]{1,3}\\.){3}[0-9]{1,3}$";
    regex_t regex;
    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        flag = ERR_IP_NOT_VALID;
    } else {
        int result = regexec(&regex, ipStr, 0, NULL, 0);
        flag = (result == 0) ? ERR_OK : ERR_IP_NOT_VALID;
        regfree(&regex);
    }
    *ipIsValid = flag;
}

int isValidTime(const char* timeString, struct tm * time)
{
    int hour   = time->tm_hour;
    int minute = time->tm_min;

    if (sscanf(timeString, "%d:%d", &hour, &minute) != 2) {
        return ERR_TIME_NOT_VALID;
    }

    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
        time->tm_hour   = 0;
        time->tm_min    = 0;
        return ERR_TIME_NOT_VALID;
    }

    time->tm_hour = hour;
    time->tm_min = minute;

    return ERR_OK;
}

int utils_check_file_exists(const char* path)
{
    struct stat buffer;
    int res = (stat(path, &buffer) == 0) ? 1 : 0;
    return res;
}
