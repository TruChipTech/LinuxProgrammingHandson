/*
 * udp_server.c — UDP echo server
 *
 * Build: make
 * Run:   ./udp_server 9090
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
static void sigint_handler(int sig) { (void)sig; running = 0; }

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    signal(SIGINT, sigint_handler);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr.s_addr = INADDR_ANY,
    };

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(fd); return 1;
    }

    printf("UDP Echo Server listening on port %d\n", port);
    printf("Press Ctrl+C to stop.\n\n");

    char buf[4096];
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        ssize_t n = recvfrom(fd, buf, sizeof(buf) - 1, 0,
                             (struct sockaddr *)&client_addr, &client_len);
        if (n < 0) {
            if (!running) break;
            perror("recvfrom");
            continue;
        }

        buf[n] = '\0';
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        printf("From %s:%d (%zd bytes): %s",
               ip, ntohs(client_addr.sin_port), n, buf);

        /* Echo back */
        sendto(fd, buf, n, 0,
               (struct sockaddr *)&client_addr, client_len);
    }

    close(fd);
    printf("\nServer stopped.\n");
    return 0;
}
