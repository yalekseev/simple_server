#include "tasks_common.h"
#include "tasks_thread.h"

#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t *threads;
static int num_threads;

static void service_tcp_requests(int server_fd) {
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

        service_single_tcp_request(client_fd);

        if (-1 == close(client_fd)) {
            syslog(LOG_ERR, "close %s", strerror(errno));
        }
    }
}

static void *service_tcp_requests_thread(void *arg) {
    int server_fd = (int)arg;
    service_tcp_requests(server_fd);
    return NULL;
}

static void service_udp_requests(int server_fd) {
    while (1) {
        service_single_udp_request(server_fd);
    }
}

static void *service_udp_requests_thread(void *arg) {
    int server_fd = (int)arg;
    service_udp_requests(server_fd);
    return NULL;
}

void spawn_thread_tasks(int tcp_service_fd, int udp_service_fd, int num_tasks) {
    assert(NULL == threads);
    assert(0 == num_threads);

    // 1 thread is to service udp requests
    num_threads = num_tasks + 1;

    threads = malloc(num_threads * sizeof(pthread_t));
    assert(NULL != threads);

    int error_code = 0;
    pthread_attr_t thread_attr;
    error_code = pthread_attr_init(&thread_attr);
    assert(0 == error_code);

    error_code = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
    assert(0 == error_code);

    int i;
    for (i = 0; i < num_threads; ++i) {
        if (i + 1 == num_threads) {
            error_code = pthread_create(&threads[i], &thread_attr, &service_udp_requests_thread, (void *)tcp_service_fd);
        } else {
            error_code = pthread_create(&threads[i], &thread_attr, &service_tcp_requests_thread, (void *)tcp_service_fd);
        }
        assert(0 == error_code);
    }

    error_code = pthread_attr_destroy(&thread_attr);
    assert(0 == error_code);
}

int continue_thread_service() {
    return 1;
}
