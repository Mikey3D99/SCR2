//
// Created by wlodi on 24.06.2023.
//

#ifndef SCR2_SERVER_H
#define SCR2_SERVER_H

#include "../task/task.h"

#define INIT_MSG_Q_ERR -1


int run_server();
int initialize_message_queue();
int destroy_message_queue();
#endif //SCR2_SERVER_H
