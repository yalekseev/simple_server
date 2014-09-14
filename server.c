#include "io.h"
#include "tasks.h"
#include "server.h"

#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

int main() {
    if (daemon(0, 0) == -1) {
        perror("daemon");
        exit(EXIT_FAILURE);
    }

    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        syslog(LOG_EMERG, "signal %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (signal(SIGHUP, SIG_IGN) == SIG_ERR) {
        syslog(LOG_EMERG, "signal %s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct addrinfo hints, *result;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Datagram socket */
    hints.ai_flags = AI_PASSIVE;     /* For wildcard IP address */
    hints.ai_protocol = 0;           /* Any protocol */
    hints.ai_canonname = NULL;
    hints.ai_addr = NULL;
    hints.ai_next = NULL;
    int ret = getaddrinfo(NULL, SERVICE, &hints, &result);
    if (ret != 0) {
        syslog(LOG_EMERG, "getaddrinfo %s", gai_strerror(ret));
        exit(EXIT_FAILURE);
    }

    int server_fd;
    struct addrinfo *p;
    for (p = result; p != NULL; p = p->ai_next) {
        server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (-1 == server_fd) {
            syslog(LOG_INFO, "socket %s", strerror(errno));
            continue;
        }

        int optval = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1) {
            syslog(LOG_INFO, "setsockopt %s", strerror(errno));
            close(server_fd);
            continue;
        }

        if (bind(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
            syslog(LOG_INFO, "bind %s", strerror(errno));
            close(server_fd);
            continue;
        }

        int backlog = BACKLOG;
        if (getenv("LISTENQ") != NULL) {
            backlog = atoi(getenv("LISTENQ"));
        }

        if (listen(server_fd, backlog) == -1) {
            syslog(LOG_INFO, "listen %s", strerror(errno));
            close(server_fd);
            continue;
        }

        break;
    }

    if (NULL == p) {
        syslog(LOG_EMERG, "Failed to bind to any addresses");
        exit(EXIT_FAILURE);
    }

    freeaddrinfo(result);

    spawn_service_tasks(server_fd, THREAD);

    while (continue_service()) {
        pause();
    }

    return 0;
}
