/*
 * mini_server.c — Custom protocol server
 *
 * Implements a TCP server using the custom mini-protocol.
 * Handles HELLO, DATA, STATUS, and BYE messages.
 *
 * Build: make
 * Run:   ./mini_server [port]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "protocol.h"

static volatile int running = 1;
static time_t start_time;
static unsigned long total_connections = 0;
static unsigned long total_messages = 0;

static void sigint_handler(int sig) { (void)sig; running = 0; }

/* Read exactly n bytes */
static ssize_t read_all(int fd, void *buf, size_t n)
{
    size_t total = 0;
    while (total < n) {
        ssize_t r = read(fd, (char *)buf + total, n - total);
        if (r <= 0) return r;
        total += r;
    }
    return total;
}

/* Write exactly n bytes */
static ssize_t write_all(int fd, const void *buf, size_t n)
{
    size_t total = 0;
    while (total < n) {
        ssize_t w = write(fd, (const char *)buf + total, n - total);
        if (w <= 0) return w;
        total += w;
    }
    return total;
}

static void send_message(int fd, uint8_t type, uint16_t seq,
                         const void *payload, uint32_t payload_len)
{
    struct proto_header hdr;
    proto_fill_header(&hdr, type, seq, payload_len);
    write_all(fd, &hdr, sizeof(hdr));
    if (payload_len > 0 && payload)
        write_all(fd, payload, payload_len);
}

static void handle_client(int fd, struct sockaddr_in *addr)
{
    char ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr->sin_addr, ip, sizeof(ip));
    printf("[%s:%d] Connected\n", ip, ntohs(addr->sin_port));
    total_connections++;

    char client_name[33] = "unknown";

    while (running) {
        struct proto_header hdr;
        ssize_t n = read_all(fd, &hdr, sizeof(hdr));
        if (n <= 0) break;

        int err = proto_validate_header(&hdr);
        if (err < 0) {
            printf("[%s] Invalid header (err=%d)\n", client_name, err);
            send_message(fd, MSG_ERROR, 0, "Bad header", 10);
            break;
        }

        uint32_t payload_len = ntohl(hdr.payload_len);
        char payload[MAX_PAYLOAD + 1];
        if (payload_len > 0) {
            n = read_all(fd, payload, payload_len);
            if (n <= 0) break;
            payload[payload_len] = '\0';
        }

        total_messages++;
        uint16_t seq = ntohs(hdr.seq);

        printf("[%s] Received %s (seq=%u, %u bytes)\n",
               client_name, proto_type_str(hdr.msg_type), seq, payload_len);

        switch (hdr.msg_type) {
        case MSG_HELLO: {
            if (payload_len >= sizeof(struct proto_hello)) {
                struct proto_hello *hello = (struct proto_hello *)payload;
                snprintf(client_name, sizeof(client_name), "%.*s",
                         (int)sizeof(hello->client_name), hello->client_name);
            }
            printf("[%s] Client identified\n", client_name);
            send_message(fd, MSG_ACK, seq, NULL, 0);
            break;
        }
        case MSG_DATA: {
            printf("[%s] Data: %.*s\n", client_name,
                   (int)payload_len, payload);
            /* Echo back as ACK */
            send_message(fd, MSG_ACK, seq, payload, payload_len);
            break;
        }
        case MSG_STATUS: {
            struct proto_status status;
            status.uptime_sec = htonl((uint32_t)(time(NULL) - start_time));
            status.connections = htonl((uint32_t)total_connections);
            status.messages = htonl((uint32_t)total_messages);
            send_message(fd, MSG_STATUS, seq, &status, sizeof(status));
            break;
        }
        case MSG_BYE: {
            printf("[%s] Client says goodbye\n", client_name);
            send_message(fd, MSG_ACK, seq, NULL, 0);
            goto done;
        }
        default:
            printf("[%s] Unknown message type: 0x%02x\n",
                   client_name, hdr.msg_type);
            send_message(fd, MSG_ERROR, seq, "Unknown type", 12);
            break;
        }
    }

done:
    printf("[%s] Disconnected\n", client_name);
}

int main(int argc, char *argv[])
{
    int port = (argc > 1) ? atoi(argv[1]) : PROTO_PORT;
    signal(SIGINT, sigint_handler);
    signal(SIGPIPE, SIG_IGN);
    start_time = time(NULL);

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
    if (listen(server_fd, 5) < 0) {
        perror("listen"); close(server_fd); return 1;
    }

    printf("Mini Protocol Server v%d on port %d\n", PROTO_VERSION, port);
    printf("Magic: 0x%08X\n", PROTO_MAGIC);
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
        handle_client(client_fd, &client_addr);
        close(client_fd);
    }

    close(server_fd);
    printf("\nServer stopped. Total: %lu connections, %lu messages\n",
           total_connections, total_messages);
    return 0;
}
