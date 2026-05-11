/*
 * tcp_client.c — TCP echo client
 *
 * Build: make
 * Run:   ./tcp_client 127.0.0.1 8080
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

    /* 1. Create socket */
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return 1;
    }

    /* 2. Connect to server */
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, server_ip, &addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address: %s\n", server_ip);
        close(fd);
        return 1;
    }

    printf("Connecting to %s:%d...\n", server_ip, port);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(fd);
        return 1;
    }
    printf("Connected! Type messages (Ctrl+D to quit):\n");

    /* 3. Send / Receive loop */
    char line[1024];
    while (fgets(line, sizeof(line), stdin) != NULL) {
        size_t len = strlen(line);

        /* Send */
        ssize_t sent = 0;
        while (sent < (ssize_t)len) {
            ssize_t w = write(fd, line + sent, len - sent);
            if (w <= 0) {
                perror("write");
                goto done;
            }
            sent += w;
        }

        /* Receive echo */
        char buf[1024];
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n <= 0) {
            printf("Server closed connection.\n");
            break;
        }
        buf[n] = '\0';
        printf("Echo: %s", buf);
    }

done:
    close(fd);
    printf("Disconnected.\n");
    return 0;
}
