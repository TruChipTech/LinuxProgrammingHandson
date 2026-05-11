/*
 * fifo_demo.c — Named Pipe (FIFO) Server/Client
 *
 * Build: gcc -Wall -g fifo_demo.c -o fifo_demo
 * Usage: ./fifo_demo server   (terminal 1)
 *        ./fifo_demo client   (terminal 2)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define FIFO_PATH "/tmp/my_fifo"
#define MAX_MSG   256

void run_server(void) {
    printf("[Server] Creating FIFO at %s\n", FIFO_PATH);

    /* Remove old FIFO if it exists */
    unlink(FIFO_PATH);

    if (mkfifo(FIFO_PATH, 0666) < 0) {
        perror("mkfifo");
        return;
    }

    printf("[Server] Waiting for client...\n");
    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd < 0) { perror("open"); return; }

    printf("[Server] Client connected.\n");

    char buf[MAX_MSG];
    ssize_t n;
    while ((n = read(fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        printf("[Server] Received: '%s'\n", buf);
        if (strcmp(buf, "quit") == 0) break;
    }

    close(fd);
    unlink(FIFO_PATH);
    printf("[Server] Done.\n");
}

void run_client(void) {
    printf("[Client] Connecting to FIFO at %s\n", FIFO_PATH);

    int fd = open(FIFO_PATH, O_WRONLY);
    if (fd < 0) {
        perror("open (is the server running?)");
        return;
    }

    printf("[Client] Connected. Type messages (or 'quit' to exit):\n");

    char buf[MAX_MSG];
    while (1) {
        printf("> ");
        if (fgets(buf, sizeof(buf), stdin) == NULL) break;

        /* Remove newline */
        buf[strcspn(buf, "\n")] = '\0';

        write(fd, buf, strlen(buf));

        if (strcmp(buf, "quit") == 0) break;
    }

    close(fd);
    printf("[Client] Done.\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <server|client>\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "server") == 0) {
        run_server();
    } else if (strcmp(argv[1], "client") == 0) {
        run_client();
    } else {
        fprintf(stderr, "Unknown mode: %s\n", argv[1]);
        return 1;
    }

    return 0;
}
