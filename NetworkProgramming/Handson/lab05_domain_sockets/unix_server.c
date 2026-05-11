/*
 * unix_server.c — Unix domain socket server (stream)
 *
 * Build: make
 * Run:   ./unix_server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/lab_unix_socket"

static volatile int running = 1;
static void sigint_handler(int sig) { (void)sig; running = 0; }

int main(void)
{
    signal(SIGINT, sigint_handler);

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    /* Remove old socket file */
    unlink(SOCKET_PATH);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(fd); return 1;
    }

    if (listen(fd, 5) < 0) {
        perror("listen"); close(fd); return 1;
    }

    printf("Unix Domain Socket Server listening on %s\n", SOCKET_PATH);
    printf("Press Ctrl+C to stop.\n\n");

    while (running) {
        struct sockaddr_un client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_fd = accept(fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) {
            if (!running) break;
            perror("accept");
            continue;
        }

        printf("Client connected (fd=%d)\n", client_fd);

        /* Echo loop */
        char buf[1024];
        ssize_t n;
        while ((n = read(client_fd, buf, sizeof(buf))) > 0) {
            printf("  Received %zd bytes: %.*s", n, (int)n, buf);
            write(client_fd, buf, n);
        }

        printf("Client disconnected\n");
        close(client_fd);
    }

    close(fd);
    unlink(SOCKET_PATH);
    printf("\nServer stopped.\n");
    return 0;
}
