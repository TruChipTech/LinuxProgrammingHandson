/*
 * epoll_server.c — High-performance multiplexed TCP server using epoll
 *
 * Build: make
 * Run:   ./epoll_server 8080
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_EVENTS 64

static volatile int running = 1;
static void sigint_handler(int sig) { (void)sig; running = 0; }

static int set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    signal(SIGINT, sigint_handler);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY,
    };

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(server_fd); return 1;
    }
    if (listen(server_fd, 128) < 0) {
        perror("listen"); close(server_fd); return 1;
    }

    set_nonblocking(server_fd);

    /* Create epoll instance */
    int epfd = epoll_create1(0);
    if (epfd < 0) { perror("epoll_create1"); close(server_fd); return 1; }

    /* Add server socket to epoll */
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

    printf("Epoll-based server on port %d\n", port);
    printf("Press Ctrl+C to stop.\n\n");

    struct epoll_event events[MAX_EVENTS];
    int num_clients = 0;

    while (running) {
        int nfds = epoll_wait(epfd, events, MAX_EVENTS, 1000);
        if (nfds < 0) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_fd) {
                /* New connection(s) */
                while (1) {
                    struct sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(server_fd,
                                           (struct sockaddr *)&client_addr, &client_len);
                    if (client_fd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break; /* No more pending connections */
                        perror("accept");
                        break;
                    }

                    set_nonblocking(client_fd);

                    ev.events = EPOLLIN | EPOLLET; /* Edge-triggered */
                    ev.data.fd = client_fd;
                    epoll_ctl(epfd, EPOLL_CTL_ADD, client_fd, &ev);
                    num_clients++;

                    char ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
                    printf("+ Client %s:%d (fd=%d, total=%d)\n",
                           ip, ntohs(client_addr.sin_port), client_fd, num_clients);
                }
            } else {
                /* Data from client */
                int fd = events[i].data.fd;
                char buf[1024];

                while (1) {
                    ssize_t n = read(fd, buf, sizeof(buf));
                    if (n < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break; /* Done reading for now */
                        /* Error */
                        goto close_client;
                    }
                    if (n == 0) {
                        /* Client disconnected */
                        goto close_client;
                    }
                    /* Echo back */
                    write(fd, buf, n);
                }
                continue;

            close_client:
                printf("- Client fd=%d disconnected (total=%d)\n", fd, num_clients - 1);
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                close(fd);
                num_clients--;
            }
        }
    }

    close(epfd);
    close(server_fd);
    printf("\nServer stopped.\n");
    return 0;
}
