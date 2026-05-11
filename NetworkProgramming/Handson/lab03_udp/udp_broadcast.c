/*
 * udp_broadcast.c — UDP broadcast sender
 *
 * Build: make
 * Run:   ./udp_broadcast 9090 "Hello everyone!"
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
        fprintf(stderr, "Usage: %s <port> <message>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    const char *msg = argv[2];

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    /* Enable broadcast */
    int bcast = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_BROADCAST, &bcast, sizeof(bcast)) < 0) {
        perror("setsockopt SO_BROADCAST");
        close(fd);
        return 1;
    }

    struct sockaddr_in bcast_addr;
    memset(&bcast_addr, 0, sizeof(bcast_addr));
    bcast_addr.sin_family = AF_INET;
    bcast_addr.sin_port = htons(port);
    inet_pton(AF_INET, "255.255.255.255", &bcast_addr.sin_addr);

    printf("Broadcasting to port %d: \"%s\"\n", port, msg);

    ssize_t sent = sendto(fd, msg, strlen(msg), 0,
                          (struct sockaddr *)&bcast_addr, sizeof(bcast_addr));
    if (sent < 0) {
        perror("sendto");
    } else {
        printf("Sent %zd bytes\n", sent);
    }

    close(fd);
    return 0;
}
