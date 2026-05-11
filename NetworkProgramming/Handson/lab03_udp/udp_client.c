/*
 * udp_client.c — UDP echo client
 *
 * Build: make
 * Run:   ./udp_client 127.0.0.1 9090
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    /* Set receive timeout */
    struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address: %s\n", server_ip);
        close(fd);
        return 1;
    }

    printf("UDP Client → %s:%d\n", server_ip, port);
    printf("Type messages (Ctrl+D to quit):\n");

    char line[1024];
    while (fgets(line, sizeof(line), stdin) != NULL) {
        size_t len = strlen(line);

        ssize_t sent = sendto(fd, line, len, 0,
                              (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (sent < 0) {
            perror("sendto");
            continue;
        }

        char buf[1024];
        ssize_t n = recvfrom(fd, buf, sizeof(buf) - 1, 0, NULL, NULL);
        if (n < 0) {
            perror("recvfrom (timeout?)");
            continue;
        }
        buf[n] = '\0';
        printf("Echo: %s", buf);
    }

    close(fd);
    return 0;
}
