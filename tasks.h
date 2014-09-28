#pragma

typedef enum {
    THREAD = 0,
    PROC = 1
} service_task_t;

typedef enum {
    SERVICE_ECHO = 0,
    SERVICE_FILE = 1
} service_t;

extern void spawn_service_tasks(int tcp_service_fd, int udp_service_fd, service_t type, service_task_t task_type, int num_tasks);

extern int continue_service();
