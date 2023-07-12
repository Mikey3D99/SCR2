cmake_minimum_required(VERSION 3.22)
project(SCR2 C)

set(CMAKE_C_STANDARD 99)

add_executable(SCR2 main.c priority_queue/priority_queue.c priority_queue/priority_queue.h task/task.c task/task.h server/server.c server/server.h client/client.c client/client.h logging_system.c logging_system.h)
