#include "tasks.h"
#include "proc_tasks.h"
#include "thread_tasks.h"

static service_task_t task_type;

void spawn_service_tasks(int server_fd, service_task_t type) {
    task_type = type;

    if (task_type == THREAD) {
        spawn_thread_tasks(server_fd);
    } else if (task_type == PROC) {
        spawn_proc_tasks(server_fd);
    }
}

int continue_service() {
    if (task_type == THREAD) {
        return continue_thread_service();
    } else if (task_type == PROC) {
        return continue_proc_service();
    } else {
        return 0;
    }
}
