#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "app/http.h"
#include "app/log.h"
#include "app/types.h"

#define BUF_SIZE 1024

bool serve(int port);

int main(void) {

    if (!serve(4221)) {
        perror("serve failed");
        return -1;
    }

    return 0;
}

bool set_socketREUSEADDR(int fd);
bool set_nonblocking(int fd);
bool run_loop_with_select(int server_fd, void *(*handle_client)(void *arg));
bool run_loop_threaded(int server_fd, void *(*handle_client)(void *arg));

void *echo_connection(void *arg) {
    size_t fd = (size_t)arg;

    char buf[1024];

    while (true) {
        ssize_t n = read(fd, buf, sizeof(buf));
        if (n > 0) {
            ssize_t total_send = 0;
            while (total_send < n) {
                ssize_t sent = write(fd, buf + total_send, n - total_send);
                if (sent < 0) {
                    perror("write");
                    close(fd);
                    return NULL;
                }
                total_send += sent;
            }
        }
        if (n == 0) {
            close(fd);
        }
        if (n < 0) {
            perror("read");
            close(fd);
            return NULL;
        }
    }
    return NULL;
}

bool serve(int port) {
    int server_fd;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // if (set_nonblocking(server_fd) != true) {
    //     perror("set_nonblocking failed");
    //     return false;
    // }

    if (set_socketREUSEADDR(server_fd) != true) {
        perror("set_socketREUSEADDR failed");
        return false;
    }

    struct sockaddr_in server_addr = {.sin_family = AF_INET,
                                      .sin_port = htons(port),
                                      .sin_addr = {.s_addr = INADDR_ANY}};

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) !=
        0) {
        perror("bind failed");
        return false;
    }

    if (listen(server_fd, 10) == -1) {
        perror("listen failed");
        return false;
    }

    LOG_INFO("Listening for connections on port %d :)", port);

    if (run_loop_threaded(server_fd, handle_http_connection) != true) {
        perror("run_loop_threaded failed");
        return false;
    }

    return true;
}

bool run_loop_threaded(int server_fd, void *(*handle_client)(void *arg)) {
    while (1) {
        ssize_t new_client_fd = accept(server_fd, NULL, NULL);
        if (new_client_fd == -1) {
            perror("accept failed");
        } else {
            pthread_t thread;
            pthread_create(&thread, NULL, handle_client, (void *)new_client_fd);
            pthread_detach(thread);
        }
    }

    close(server_fd);
    return true;
}

bool run_loop_with_select(int server_fd, void *(*handle_client)(void *arg)) {
    fd_set current_fd_list, read_fd_list;

    FD_ZERO(&current_fd_list);
    FD_SET(server_fd, &current_fd_list);
    int max_fd = server_fd;
    int fd_list[FD_SETSIZE];
    for (int i = 0; i < FD_SETSIZE; ++i)
        fd_list[i] = -1;

    while (1) {
        read_fd_list = current_fd_list;
        int number_of_ready_fds =
            select(max_fd + 1, &read_fd_list, NULL, NULL, NULL);
        if (number_of_ready_fds < 0) {
            perror("select failed");

            return false;
        }

        if (FD_ISSET(server_fd, &read_fd_list)) {
            struct sockaddr_in client_addr;

            socklen_t addr_size = sizeof(client_addr);
            int new_client_fd =
                accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
            if (new_client_fd == -1) {
                perror("accept failed");
            } else {
                set_nonblocking(new_client_fd);

                // add to master set
                FD_SET(new_client_fd, &current_fd_list);
                if (new_client_fd > max_fd)
                    max_fd = new_client_fd;

                // track
                for (int i = 0; i < FD_SETSIZE; ++i) {
                    if (fd_list[i] < 0) {
                        fd_list[i] = new_client_fd;
                        break;
                    }
                }
            }
            if (--number_of_ready_fds == 0)
                continue;
        }

        for (int i = 0; i < FD_SETSIZE; i++) {
            size_t fd = fd_list[i];
            if (fd < 0)
                continue;

            if (FD_ISSET(fd, &read_fd_list)) {
                handle_client((void *)fd);
                FD_CLR(fd, &current_fd_list);
                fd_list[i] = -1;
                if (--number_of_ready_fds == 0)
                    break;
            }
        }
    }
}

bool set_socketREUSEADDR(int fd) {

    // Set socket to reuse address
    int opt = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt failed");
        return false;
    }

#if defined(__APPLE__) || defined(__FreeBSD__)
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)) == -1) {
        perror("setsockopt SO_REUSEPORT failed");
        return false;
    }
#endif

    return true;
}

bool set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl(F_GETFL)");
        return false;
    }
    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl(F_SETFL)");
        return false;
    }
    return true;
}
