#pragma once

extern void handle_requests(int server_fd);

extern void start_handle_threads(int num_threads, int server_fd);
