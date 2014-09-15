#pragma

typedef enum {
    THREAD = 0,
    PROC = 1
} service_task_t;

extern void spawn_service_tasks(int server_fd, service_task_t type, int num_tasks);

extern int continue_service();
