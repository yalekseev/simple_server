#include "io.h"
#include "handler.h"

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void handle_single_request(int socket_fd) {
    /* set send/receive timeouts */
    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        close(socket_fd);
        return;
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) == -1) {
        close(socket_fd);
        return;
    }

    char file_name[MAX_FILE_NAME + 1];
    ssize_t bytes_read = readn(socket_fd, file_name, MAX_FILE_NAME);
    if (bytes_read <= 0) {
        close(socket_fd);
        return;
    }

    file_name[bytes_read] = '\0';

    int file_fd = open(file_name, O_RDONLY);
    if (-1 == file_fd) {
        close(socket_fd);
        return;
    }

    sendfile(socket_fd, file_fd, NULL, MAX_FILE_SIZE);

    close(file_fd);
    close(socket_fd);
}

void handle_requests(int server_fd) {
    while (1) {
        pthread_mutex_lock(&mutex);

        int client_fd = accept(server_fd, NULL, NULL);
        if (-1 == client_fd) {
            syslog(LOG_ERR, "accept %s", strerror(errno));
            continue;
        }

        pthread_mutex_unlock(&mutex);

        handle_single_request(client_fd);
    }
}

void *handle_requests_thread(void *arg) {
    int server_fd = (int)arg;
    handle_requests(server_fd);
    return NULL;
}

void start_handle_threads(int num_threads, int server_fd) {
    int i;
    for (i = 0; i < num_threads; ++i) {
        int error_code;

        pthread_attr_t attr;
        error_code = pthread_attr_init(&attr);
        if (0 != error_code) {
            syslog(LOG_ERR, "pthread_attr_init %s", strerror(error_code));
            return;
        }

        error_code = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (0 != error_code) {
            syslog(LOG_ERR, "pthread_attr_setdetachstate %s", strerror(error_code));
            return;
        }

        pthread_t thread;
        error_code = pthread_create(&thread, &attr, &handle_requests_thread, (void *)server_fd);
        if (0 != error_code) {
            syslog(LOG_ERR, "pthread_create %s", strerror(error_code));
        }

        error_code = pthread_attr_destroy(&attr);
        if (0 != error_code) {
            syslog(LOG_ERR, "pthread_attr_destroy %s", strerror(error_code));
        }
    }
}
