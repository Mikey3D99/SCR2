//
// Created by wlodi on 24.06.2023.
//

#ifndef SCR2_CLIENT_H
#define SCR2_CLIENT_H
#include <stdio.h>
#include <sys/msg.h>
#include "../task/task.h"


int run_client(int argc, char *argv[]);
int connect_to_message_queue();
int destroy_message_queue();

#endif //SCR2_CLIENT_H
