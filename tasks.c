#include "tasks.h"
#include "tasks_proc.h"
#include "tasks_thread.h"

static service_task_t task_type;

void spawn_service_tasks(int server_fd, service_task_t type, int num_tasks) {
    task_type = type;

    if (task_type == THREAD) {
        spawn_thread_tasks(server_fd, num_tasks);
    } else if (task_type == PROC) {
        spawn_proc_tasks(server_fd, num_tasks);
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
