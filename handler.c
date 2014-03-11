#include "io.h"
#include "handler.h"

#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sendfile.h>

void *handle_request_helper(void *arg) {
    int socket_fd = (int)arg;

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

void handle_request(int socket_fd) {
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
    error_code = pthread_create(&thread, &attr, &handle_request_helper, (void *)socket_fd);
    if (0 != error_code) {
        syslog(LOG_ERR, "pthread_create %s", strerror(error_code));
    }

    error_code = pthread_attr_destroy(&attr);
    if (0 != error_code) {
        syslog(LOG_ERR, "pthread_attr_destroy %s", strerror(error_code));
    }
}
