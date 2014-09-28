#pragma once

extern void spawn_thread_tasks(int tcp_service_fd, int udp_service_fd, int num_tasks);

extern int continue_thread_service();
