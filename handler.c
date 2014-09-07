#include "io.h"
#include "handler.h"

#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>

enum { NUM_THREADS = 12 };

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t *threads;
static int threads_count;

static void handle_single_request(int socket_fd) {
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

    if (-1 == sendfile(socket_fd, file_fd, NULL, MAX_FILE_SIZE)) {
        syslog(LOG_ERR, "sendfile %s", strerror(errno));
    }

    if (-1 == close(file_fd)) {
        syslog(LOG_ERR, "close %s", strerror(errno));
    }
}

static void handle_requests(int server_fd) {
    while (1) {
        pthread_mutex_lock(&mutex);

        int client_fd = accept(server_fd, NULL, NULL);
        if (-1 == client_fd && errno == EINTR) {
            continue;
        } else if (-1 == client_fd && errno == ECONNABORTED) {
            continue;
        } else if (-1 == client_fd) {
            syslog(LOG_ERR, "accept %s", strerror(errno));
            continue;
        }

        pthread_mutex_unlock(&mutex);

        handle_single_request(client_fd);

        if (-1 == close(client_fd)) {
            syslog(LOG_ERR, "close %s", strerror(errno));
        }
    }
}

static void *handle_requests_thread(void *arg) {
    int server_fd = (int)arg;
    handle_requests(server_fd);
    return NULL;
}

void spawn_service_tasks(int server_fd) {
    assert(NULL == threads);
    assert(0 == threads_count);

    threads_count = NUM_THREADS;

    threads = malloc(threads_count * sizeof(pthread_t));
    assert(NULL != threads);

    int error_code = 0;
    pthread_attr_t thread_attr;
    error_code = pthread_attr_init(&thread_attr);
    assert(0 == error_code);

    error_code = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    assert(0 == error_code);

    int i;
    for (i = 0; i < threads_count; ++i) {
        error_code = pthread_create(&threads[i], &thread_attr, &handle_requests_thread, (void *)server_fd);
        assert(0 == error_code);
    }

    error_code = pthread_attr_destroy(&thread_attr);
    assert(0 == error_code);
}
