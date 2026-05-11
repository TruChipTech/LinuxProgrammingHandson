/*
 * tcp_server_concurrent.c — Concurrent TCP server using fork()
 *
 * Build: make
 * Run:   ./tcp_server_concurrent 8080
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static volatile int running = 1;

static void sigint_handler(int sig) { (void)sig; running = 0; }
static void sigchld_handler(int sig) { (void)sig; while (waitpid(-1, NULL, WNOHANG) > 0); }

static void handle_client(int fd, struct sockaddr_in *addr)
{
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
    printf("[PID %d] Handling client %s:%d\n", getpid(), ip, ntohs(addr->sin_port));

    char buf[1024];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        /* Echo back */
        ssize_t sent = 0;
        while (sent < n) {
            ssize_t w = write(fd, buf + sent, n - sent);
            if (w <= 0) return;
            sent += w;
        }
    }

    printf("[PID %d] Client %s:%d disconnected\n", getpid(), ip, ntohs(addr->sin_port));
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);

    signal(SIGINT, sigint_handler);
    signal(SIGCHLD, sigchld_handler);

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

    printf("Concurrent TCP Server on port %d (fork-per-client)\n", port);
    printf("Press Ctrl+C to stop.\n\n");

    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (!running) break;
            perror("accept");
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            close(client_fd);
            continue;
        }

        if (pid == 0) {
            /* Child process — handle client */
            close(server_fd);
            handle_client(client_fd, &client_addr);
            close(client_fd);
            exit(0);
        }

        /* Parent — continue accepting */
        close(client_fd);
    }

    close(server_fd);
    printf("\nServer shut down.\n");
    return 0;
}
