/*
 * select_server.c — Multiplexed TCP server using select()
 *
 * Build: make
 * Run:   ./select_server 8080
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 64

static volatile int running = 1;
static void sigint_handler(int sig) { (void)sig; running = 0; }

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
    if (listen(server_fd, 10) < 0) {
        perror("listen"); close(server_fd); return 1;
    }

    printf("Select-based server on port %d (max %d clients)\n", port, MAX_CLIENTS);
    printf("Press Ctrl+C to stop.\n\n");

    int client_fds[MAX_CLIENTS];
    int num_clients = 0;

    while (running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int maxfd = server_fd;

        for (int i = 0; i < num_clients; i++) {
            FD_SET(client_fds[i], &readfds);
            if (client_fds[i] > maxfd)
                maxfd = client_fds[i];
        }

        struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
        int ret = select(maxfd + 1, &readfds, NULL, NULL, &tv);
        if (ret < 0) {
            if (!running) break;
            perror("select");
            continue;
        }
        if (ret == 0) continue;

        /* New connection? */
        if (FD_ISSET(server_fd, &readfds)) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
            if (client_fd >= 0) {
                if (num_clients < MAX_CLIENTS) {
                    client_fds[num_clients++] = client_fd;
                    char ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
                    printf("+ Client %s:%d (fd=%d, total=%d)\n",
                           ip, ntohs(client_addr.sin_port), client_fd, num_clients);
                } else {
                    printf("Max clients reached, rejecting.\n");
                    close(client_fd);
                }
            }
        }

        /* Data from existing clients? */
        for (int i = 0; i < num_clients; i++) {
            if (FD_ISSET(client_fds[i], &readfds)) {
                char buf[1024];
                ssize_t n = read(client_fds[i], buf, sizeof(buf));
                if (n <= 0) {
                    /* Client disconnected */
                    printf("- Client fd=%d disconnected (total=%d)\n",
                           client_fds[i], num_clients - 1);
                    close(client_fds[i]);
                    /* Remove from array */
                    client_fds[i] = client_fds[num_clients - 1];
                    num_clients--;
                    i--;
                } else {
                    /* Echo back */
                    write(client_fds[i], buf, n);
                }
            }
        }
    }

    for (int i = 0; i < num_clients; i++)
        close(client_fds[i]);
    close(server_fd);
    printf("\nServer stopped.\n");
    return 0;
}
