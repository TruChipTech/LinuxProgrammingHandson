/*
 * unix_client.c — Unix domain socket client (stream)
 *
 * Build: make
 * Run:   ./unix_client
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

#define SOCKET_PATH "/tmp/lab_unix_socket"

int main(void)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    printf("Connecting to %s...\n", SOCKET_PATH);
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(fd);
        return 1;
    }
    printf("Connected! Type messages (Ctrl+D to quit):\n");

    char line[1024];
    while (fgets(line, sizeof(line), stdin) != NULL) {
        size_t len = strlen(line);
        write(fd, line, len);

        char buf[1024];
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n <= 0) {
            printf("Server closed connection.\n");
            break;
        }
        buf[n] = '\0';
        printf("Echo: %s", buf);
    }

    close(fd);
    printf("Disconnected.\n");
    return 0;
}
