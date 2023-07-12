//
// Created by wlodi on 12.07.2023.
//

#ifndef SCR2_LOGGING_SYSTEM_H
#define SCR2_LOGGING_SYSTEM_H
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
typedef enum { MIN, STANDARD, MAX } log_level_t;
#endif //SCR2_LOGGING_SYSTEM_H
