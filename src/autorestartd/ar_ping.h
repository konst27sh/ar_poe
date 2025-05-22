//
// Created by sheverdin on 5/22/25.
//

#ifndef TF_AUTORESTART_AR_PING_H
#define TF_AUTORESTART_AR_PING_H

#include "../global_includs.h"

typedef enum
{
    PING_OK         = 0,
    PING_ERR_TEST   = 1
} ping_error_t;

ping_error_t ping_host(const char *host);

#endif //TF_AUTORESTART_AR_PING_H
