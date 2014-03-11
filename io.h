#pragma once

#include <unistd.h>

extern ssize_t readn(int fd, void *buf, size_t count);

extern ssize_t writen(int fd, const void *buf, size_t count);
