#include "io.h"

#include <errno.h>
#include <unistd.h>

ssize_t readn(int fd, void *buf, size_t count) {
    ssize_t total_bytes_read = 0;

    while (0 < count) {
        ssize_t bytes_read = read(fd, buf, count);
        if (-1 == bytes_read && errno == EINTR) {
            continue;
        } else if (-1 == bytes_read) {
            return -1;
        } else if (0 == bytes_read) {
            return total_bytes_read;
        }

        total_bytes_read += bytes_read;
        count -= bytes_read;
        buf += bytes_read;
    }

    return total_bytes_read;
}

ssize_t writen(int fd, const void *buf, size_t count) {
    ssize_t total_bytes_written = 0;

    while (0 < count) {
        ssize_t bytes_written = write(fd, buf, count);
        if (-1 == bytes_written && errno == EINTR) {
            continue;
        } else if (bytes_written <= 0) {
            return -1;
        }

        total_bytes_written += bytes_written;
        count -= bytes_written;
        buf += bytes_written;
    }

    return total_bytes_written;
}
