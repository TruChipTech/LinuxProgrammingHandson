/*
 * buggy_client.c — TCP client with intentional bugs
 *
 * Part of the Capstone Debugging Project.
 * Contains: stack overflow, uninitialized var, resource leak
 *
 * Build: gcc -g -O0 -o buggy_client buggy_client.c
 * Run:   ./buggy_client localhost 8080
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define BUF_SIZE 128

/* BUG: Stack buffer overflow when input is large */
void format_message(const char *input, char *output)
{
    char temp[32];
    /* BUG: No bounds checking — overflow if input > 31 chars */
    strcpy(temp, input);
    sprintf(output, "MSG: %s\n", temp);
}

int connect_to_server(const char *host, int port)
{
    int fd;
    struct sockaddr_in addr;
    struct hostent *he;

    he = gethostbyname(host);
    if (!he) {
        fprintf(stderr, "Cannot resolve %s\n", host);
        return -1;
    }

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr, he->h_length);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        /* BUG: Socket fd is leaked on error — close(fd) missing */
        return -1;
    }

    return fd;
}

int main(int argc, char **argv)
{
    const char *host = argc > 1 ? argv[1] : "localhost";
    int port = argc > 2 ? atoi(argv[2]) : 8080;
    int fd;
    int status;  /* BUG: Uninitialized variable used in condition */
    char buf[BUF_SIZE];
    char formatted[BUF_SIZE];

    printf("Connecting to %s:%d...\n", host, port);

    fd = connect_to_server(host, port);
    if (fd < 0) {
        fprintf(stderr, "Connection failed\n");
        return 1;
    }

    printf("Connected! Type messages (Ctrl+D to quit):\n");

    /* BUG: Using uninitialized 'status' */
    if (status > 0) {
        printf("(Previous session had %d messages)\n", status);
    }

    while (fgets(buf, sizeof(buf), stdin)) {
        /* Remove newline */
        buf[strcspn(buf, "\n")] = '\0';

        /* BUG: Overflow if input > 31 chars */
        format_message(buf, formatted);

        if (write(fd, formatted, strlen(formatted)) < 0) {
            perror("write");
            break;
        }

        /* Read echo response */
        ssize_t n = read(fd, buf, sizeof(buf) - 1);
        if (n <= 0) break;
        buf[n] = '\0';
        printf("Server: %s", buf);
    }

    close(fd);
    printf("Disconnected.\n");
    return 0;
}
