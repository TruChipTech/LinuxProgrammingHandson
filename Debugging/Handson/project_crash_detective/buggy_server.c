/*
 * buggy_server.c — Multi-threaded TCP server with intentional bugs
 *
 * Part of the Capstone Debugging Project.
 * Contains: data race, memory leak, use-after-free
 *
 * Build: gcc -g -O0 -o buggy_server buggy_server.c -lpthread
 * Run:   ./buggy_server 8080
 * Test:  echo "hello" | nc localhost 8080
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define BUF_SIZE    256

/* BUG: No mutex protecting this shared counter → data race */
static int connection_count = 0;

struct client_info {
    int fd;
    struct sockaddr_in addr;
};

void *handle_client(void *arg)
{
    struct client_info *client = (struct client_info *)arg;
    char buf[BUF_SIZE];
    ssize_t n;

    /* BUG: Data race on connection_count */
    connection_count++;
    printf("[%d] Client connected from %s:%d (total: %d)\n",
           client->fd,
           inet_ntoa(client->addr.sin_addr),
           ntohs(client->addr.sin_port),
           connection_count);

    /* BUG: Memory leak — allocates log buffer but never frees it */
    char *log_buf = malloc(512);
    snprintf(log_buf, 512, "Connection from %s:%d",
             inet_ntoa(client->addr.sin_addr),
             ntohs(client->addr.sin_port));
    /* log_buf is never freed! */

    /* Echo loop */
    while ((n = read(client->fd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        printf("[%d] Received: %s", client->fd, buf);

        /* Echo back with prefix */
        char response[BUF_SIZE + 32];
        snprintf(response, sizeof(response), "ECHO(%d): %s", connection_count, buf);
        write(client->fd, response, strlen(response));
    }

    close(client->fd);

    /* BUG: Data race on connection_count */
    connection_count--;
    printf("[%d] Client disconnected (total: %d)\n", client->fd, connection_count);

    /* BUG: Use-after-free — client is freed here but could be accessed by main */
    free(client);

    return NULL;
}

int main(int argc, char **argv)
{
    int port = argc > 1 ? atoi(argv[1]) : 8080;
    int server_fd;
    struct sockaddr_in server_addr;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    printf("Server listening on port %d\n", port);

    while (1) {
        struct client_info *client = malloc(sizeof(*client));
        socklen_t addr_len = sizeof(client->addr);

        client->fd = accept(server_fd, (struct sockaddr *)&client->addr, &addr_len);
        if (client->fd < 0) {
            perror("accept");
            free(client);
            continue;
        }

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, client);
        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}
