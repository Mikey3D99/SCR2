//
// Created by wlodi on 12.07.2023.
//

#include "logging_system.h"


static int log_fd;
static bool is_logging_enabled;
static log_level_t current_log_level;
static pthread_mutex_t log_mutex;
static void (*state_dump_callback)(const char *);

void handle_dump_signal(int sig, siginfo_t *info, void *ucontext) {
    (void)sig;
    (void)info;
    (void)ucontext;

    static int dump_count = 0;

    char file_name[128];
    snprintf(file_name, sizeof(file_name), "dump_%d_%d.txt", getpid(), dump_count++);
    int dump_fd = open(file_name, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (dump_fd == -1) {
        perror("Error creating dump file");
        return;
    }

    if (state_dump_callback) {
        char *dump_data = NULL;
        state_dump_callback(&dump_data);
        if (dump_data) {
            write(dump_fd, dump_data, strlen(dump_data));
            free(dump_data);
        }
    }

    close(dump_fd);
}


void handle_logging_signal(int sig, siginfo_t *info, void *ucontext) {
    (void)ucontext;

    if (sig == info->si_value.sival_int) {
        is_logging_enabled = !is_logging_enabled;
    } else {
        current_log_level = (log_level_t)info->si_value.sival_int;
    }
}

void setup_signals() {
    struct sigaction dump_action;
    memset(&dump_action, 0, sizeof(dump_action));
    dump_action.sa_sigaction = handle_dump_signal;
    dump_action.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGRTMIN, &dump_action, NULL);

    struct sigaction log_action;
    memset(&log_action, 0, sizeof(log_action));
    log_action.sa_sigaction = handle_logging_signal;
    log_action.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGRTMIN + 1, &log_action, NULL);
}

void init_logging(const char *log_file, log_level_t log_level, void (*dump_cb)(const char **)) {
    log_fd = open(log_file, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (log_fd == -1) {
        perror("Error opening log file");
        exit(1);
    }

    is_logging_enabled = true;
    current_log_level = log_level;
    state_dump_callback = dump_cb;
    pthread_mutex_init(&log_mutex, NULL);

    setup_signals();
}

void log_message(log_level_t importance, const char *message) {
    if (!is_logging_enabled || importance > current_log_level) {
        return;
    }

    pthread_mutex_lock(&log_mutex);
    write(log_fd, message, strlen(message));
    write(log_fd, "\n", 1);
    pthread_mutex_unlock(&log_mutex);
}

void close_logging() {
    close(log_fd);
    pthread_mutex_destroy(&log_mutex);
}


// Example state dump callback function
void state_dump(const char **dump_data) {
    // Allocate memory and populate with the application state
    *dump_data = strdup("Current application state: A simple example");
}
