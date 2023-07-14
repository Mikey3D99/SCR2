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
void init_logging(const char *log_file, log_level_t log_level, void (*dump_cb)(const char *));
void setup_signals();
void handle_logging_signal(int sig, siginfo_t *info, void *ucontext);
void handle_dump_signal(int sig, siginfo_t *info, void *ucontext);
void log_message(log_level_t importance, const char *message);
void close_logging();
#endif //SCR2_LOGGING_SYSTEM_H
