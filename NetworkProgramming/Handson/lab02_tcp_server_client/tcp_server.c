/*
 * tcp_server.c — Iterative TCP echo server
 *
 * Build: make
 * Run:   ./tcp_server 8080
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static volatile int running = 1;

static void sigint_handler(int sig)
{
    (void)sig;
    running = 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "Invalid port: %s\n", argv[1]);
        return 1;
    }

    signal(SIGINT, sigint_handler);

    /* 1. Create socket */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    /* 2. Set SO_REUSEADDR to avoid "Address already in use" */
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* 3. Bind to address and port */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    /* 4. Listen for connections */
    if (listen(server_fd, 5) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("TCP Echo Server listening on port %d\n", port);
    printf("Press Ctrl+C to stop.\n\n");

    while (running) {
        /* 5. Accept a connection */
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (!running) break;
            perror("accept");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        printf("Client connected: %s:%d\n", client_ip, ntohs(client_addr.sin_port));

        /* 6. Echo loop: read → write */
        char buf[1024];
        ssize_t n;
        while ((n = read(client_fd, buf, sizeof(buf))) > 0) {
            printf("  Received %zd bytes: %.*s", n, (int)n, buf);
            /* Echo back */
            ssize_t sent = 0;
            while (sent < n) {
                ssize_t w = write(client_fd, buf + sent, n - sent);
                if (w <= 0) break;
                sent += w;
            }
        }

        /* 7. Close client */
        printf("Client disconnected: %s:%d\n", client_ip, ntohs(client_addr.sin_port));
        close(client_fd);
    }

    close(server_fd);
    printf("\nServer shut down.\n");
    return 0;
}
