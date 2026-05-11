/*
 * mini_client.c — Custom protocol client
 *
 * Connects to the mini protocol server and exchanges messages.
 *
 * Build: make
 * Run:   ./mini_client [server_ip] [port]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "protocol.h"

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

static int recv_message(int fd, struct proto_header *hdr,
                        char *payload, uint32_t max_payload)
{
    ssize_t n = read_all(fd, hdr, sizeof(*hdr));
    if (n <= 0) return -1;

    int err = proto_validate_header(hdr);
    if (err < 0) {
        fprintf(stderr, "Invalid response header (err=%d)\n", err);
        return -1;
    }

    uint32_t plen = ntohl(hdr->payload_len);
    if (plen > 0) {
        if (plen > max_payload) return -1;
        n = read_all(fd, payload, plen);
        if (n <= 0) return -1;
    }
    return 0;
}

int main(int argc, char *argv[])
{
    const char *server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    int port = (argc > 2) ? atoi(argv[2]) : PROTO_PORT;

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

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
        perror("connect"); close(fd); return 1;
    }
    printf("Connected!\n\n");

    struct proto_header resp_hdr;
    char resp_payload[MAX_PAYLOAD + 1];
    uint16_t seq = 0;

    /* 1. Send HELLO */
    struct proto_hello hello;
    memset(&hello, 0, sizeof(hello));
    snprintf(hello.client_name, sizeof(hello.client_name), "test_client_%d", getpid());
    printf("→ Sending HELLO (name: %s)\n", hello.client_name);
    send_message(fd, MSG_HELLO, ++seq, &hello, sizeof(hello));

    if (recv_message(fd, &resp_hdr, resp_payload, MAX_PAYLOAD) == 0) {
        printf("← Received %s (seq=%u)\n\n",
               proto_type_str(resp_hdr.msg_type), ntohs(resp_hdr.seq));
    }

    /* 2. Send DATA messages interactively */
    printf("Commands: <text> = send data, /status = get status, /quit = disconnect\n\n");

    char line[1024];
    while (fgets(line, sizeof(line), stdin) != NULL) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
        if (len == 0) continue;

        if (strcmp(line, "/status") == 0) {
            /* Request status */
            printf("→ Requesting STATUS\n");
            send_message(fd, MSG_STATUS, ++seq, NULL, 0);

            if (recv_message(fd, &resp_hdr, resp_payload, MAX_PAYLOAD) == 0) {
                if (resp_hdr.msg_type == MSG_STATUS &&
                    ntohl(resp_hdr.payload_len) >= sizeof(struct proto_status)) {
                    struct proto_status *st = (struct proto_status *)resp_payload;
                    printf("← STATUS: uptime=%us, connections=%u, messages=%u\n\n",
                           ntohl(st->uptime_sec), ntohl(st->connections),
                           ntohl(st->messages));
                }
            }
        } else if (strcmp(line, "/quit") == 0) {
            /* Send BYE */
            printf("→ Sending BYE\n");
            send_message(fd, MSG_BYE, ++seq, NULL, 0);
            recv_message(fd, &resp_hdr, resp_payload, MAX_PAYLOAD);
            printf("← Received %s\n", proto_type_str(resp_hdr.msg_type));
            break;
        } else {
            /* Send DATA */
            printf("→ Sending DATA: %s\n", line);
            send_message(fd, MSG_DATA, ++seq, line, len);

            if (recv_message(fd, &resp_hdr, resp_payload, MAX_PAYLOAD) == 0) {
                uint32_t plen = ntohl(resp_hdr.payload_len);
                resp_payload[plen] = '\0';
                printf("← %s: %s\n\n",
                       proto_type_str(resp_hdr.msg_type), resp_payload);
            }
        }
    }

    close(fd);
    printf("Disconnected.\n");
    return 0;
}
