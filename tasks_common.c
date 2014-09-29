#include "io.h"
#include "tasks.h"
#include "tasks_proc.h"
#include "tasks_common.h"
#include "tasks_thread.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>

static void service_tcp_file_request(int socket_fd);
static void service_udp_file_request(int socket_fd);
static void service_tcp_echo_request(int socket_fd);
static void service_udp_echo_request(int socket_fd);

static service_task_t service_task_type;
static service_t service_type;

void spawn_service_tasks(int tcp_service_fd, int udp_service_fd, service_t type, service_task_t task_type, int num_tasks) {
    service_type = type;
    service_task_type = task_type;

    if (service_task_type == THREAD) {
        spawn_thread_tasks(tcp_service_fd, udp_service_fd, num_tasks);
    } else if (service_task_type == PROC) {
        spawn_proc_tasks(tcp_service_fd, udp_service_fd, num_tasks);
    } else {
        syslog(LOG_EMERG, "Unknown task_type: %d", (int)service_task_type);
        exit(EXIT_FAILURE);
    }
}

int continue_service() {
    if (service_task_type == THREAD) {
        return continue_thread_service();
    } else if (service_task_type == PROC) {
        return continue_proc_service();
    } else {
        syslog(LOG_EMERG, "Unknown task_type: %d", (int)service_task_type);
        return 0;
    }
}

void service_single_tcp_request(int socket_fd) {
    if (service_type == SERVICE_ECHO) {
        service_tcp_echo_request(socket_fd);
    } else if (service_type == SERVICE_FILE) {
        service_tcp_file_request(socket_fd);
    } else {
        syslog(LOG_ERR, "Unkown service_type: %d", (int)service_type);
    }
}

void service_single_udp_request(int socket_fd) {
    if (service_type == SERVICE_ECHO) {
        service_udp_echo_request(socket_fd);
    } else if (service_type == SERVICE_FILE) {
        service_udp_file_request(socket_fd);
    } else {
        syslog(LOG_ERR, "Unkown service_type: %d", (int)service_type);
    }
}

static void service_tcp_file_request(int socket_fd) {
    /* set send/receive timeouts */
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        return;
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
        return;
    }

    char file_name[MAX_FILE_NAME + 1];
    ssize_t bytes_read = readn(socket_fd, file_name, MAX_FILE_NAME);
    if (-1 == bytes_read) {
        syslog(LOG_ERR, "readn %s", strerror(errno));
        return;
    }

    file_name[bytes_read] = '\0';

    int file_fd = open(file_name, O_RDONLY);
    if (-1 == file_fd) {
        if (errno != ENOENT) {
            syslog(LOG_ERR, "open %s", strerror(errno));
        }
        return;
    }

    if (-1 == sendfile(socket_fd, file_fd, NULL, MAX_FILE_SIZE)) {
        syslog(LOG_ERR, "sendfile %s", strerror(errno));
    }

    if (-1 == close(file_fd)) {
        syslog(LOG_ERR, "close %s", strerror(errno));
    }
}

static void service_udp_file_request(int socket_fd) {
    /* set send/receive timeouts */
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
        return;
    }

    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    char file_name[MAX_FILE_NAME + 1];
    ssize_t bytes_read = recvfrom(socket_fd, file_name, MAX_FILE_NAME, 0, (struct sockaddr *)&addr, &len);
    if (-1 == bytes_read) {
        syslog(LOG_ERR, "recvfrom %s", strerror(errno));
        return;
    }

    file_name[bytes_read] = '\0';

    int file_fd = open(file_name, O_RDONLY);
    if (-1 == file_fd) {
        if (errno != ENOENT) {
            syslog(LOG_ERR, "open %s", strerror(errno));
        }
        return;
    }

    char buf[BUF_SIZE];

    while (1) {
        ssize_t bytes_read = readn(file_fd, buf, BUF_SIZE);
        if (-1 == bytes_read) {
            syslog(LOG_ERR, "readn %s", strerror(errno));
            break;
        } else if (0 == bytes_read) {
            break;
        }

        ssize_t bytes_written = sendto(socket_fd, buf, bytes_read, 0, (struct sockaddr *)&addr, len);
        if (bytes_written != bytes_read) {
            syslog(LOG_ERR, "sendto %s", strerror(errno));
            break;
        }
    }

    if (-1 == close(file_fd)) {
        syslog(LOG_ERR, "close %s", strerror(errno));
    }
}

static void service_tcp_echo_request(int socket_fd) {
    /* set send/receive timeouts */
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        return;
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
        return;
    }

    char buf[BUF_SIZE];

    while (1) {
        ssize_t bytes_read = read_line(socket_fd, buf, BUF_SIZE);
        if (-1 == bytes_read) {
            syslog(LOG_ERR, "read_line %s", strerror(errno));
            return;
        } else if (0 == bytes_read) {
            return;
        }

        ssize_t bytes_written = writen(socket_fd, buf, bytes_read);
        if (bytes_written != bytes_read) {
            syslog(LOG_ERR, "writen %s", strerror(errno));
            return;
        }
    }
}

static void service_udp_echo_request(int socket_fd) {
    /* set send/receive timeouts */
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;

    if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
        return;
    }

    char file_name[MAX_FILE_NAME + 1];
    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    char buf[BUF_SIZE];

    ssize_t bytes_read = recvfrom(socket_fd, buf, BUF_SIZE, 0, (struct sockaddr *)&addr, &len);
    if (bytes_read == -1) {
        syslog(LOG_ERR, "recvfrom %s", strerror(errno));
        return;
    }

    ssize_t bytes_written = sendto(socket_fd, buf, bytes_read, 0, (struct sockaddr *)&addr, len);
    if (bytes_written != bytes_read) {
        syslog(LOG_ERR, "sendto %s", strerror(errno));
        return;
    }
}
