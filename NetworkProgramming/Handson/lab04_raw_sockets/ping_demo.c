/*
 * ping_demo.c — ICMP ping implementation using raw sockets
 *
 * Build: make
 * Run:   sudo ./ping_demo 8.8.8.8
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

static volatile int running = 1;
static void sigint_handler(int sig) { (void)sig; running = 0; }

static uint16_t compute_checksum(void *data, int len)
{
    uint32_t sum = 0;
    uint16_t *ptr = (uint16_t *)data;

    while (len > 1) {
        sum += *ptr++;
        len -= 2;
    }
    if (len == 1) {
        sum += *(uint8_t *)ptr;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    return (uint16_t)~sum;
}

static double time_diff_ms(struct timeval *start, struct timeval *end)
{
    return (end->tv_sec - start->tv_sec) * 1000.0 +
           (end->tv_usec - start->tv_usec) / 1000.0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <target_ip> [count]\n", argv[0]);
        return 1;
    }

    const char *target = argv[1];
    int max_count = (argc > 2) ? atoi(argv[2]) : 4;

    signal(SIGINT, sigint_handler);

    /* Create raw ICMP socket */
    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (fd < 0) {
        perror("socket (need root/CAP_NET_RAW)");
        return 1;
    }

    /* Set receive timeout */
    struct timeval tv = { .tv_sec = 2, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    if (inet_pton(AF_INET, target, &dest.sin_addr) <= 0) {
        fprintf(stderr, "Invalid address: %s\n", target);
        close(fd);
        return 1;
    }

    printf("PING %s: %d data bytes\n", target, 56);

    uint16_t id = (uint16_t)(getpid() & 0xFFFF);
    int sent = 0, received = 0;
    double min_rtt = 99999, max_rtt = 0, total_rtt = 0;

    for (int seq = 1; seq <= max_count && running; seq++) {
        /* Build ICMP Echo Request */
        struct {
            struct icmphdr hdr;
            struct timeval timestamp;
            char padding[48]; /* 56 bytes total payload */
        } packet;

        memset(&packet, 0, sizeof(packet));
        packet.hdr.type = ICMP_ECHO;
        packet.hdr.code = 0;
        packet.hdr.un.echo.id = htons(id);
        packet.hdr.un.echo.sequence = htons(seq);
        gettimeofday(&packet.timestamp, NULL);
        packet.hdr.checksum = 0;
        packet.hdr.checksum = compute_checksum(&packet, sizeof(packet));

        struct timeval send_time;
        gettimeofday(&send_time, NULL);

        ssize_t n = sendto(fd, &packet, sizeof(packet), 0,
                           (struct sockaddr *)&dest, sizeof(dest));
        if (n < 0) {
            perror("sendto");
            continue;
        }
        sent++;

        /* Receive reply */
        unsigned char recv_buf[1024];
        struct sockaddr_in from;
        socklen_t from_len = sizeof(from);

        n = recvfrom(fd, recv_buf, sizeof(recv_buf), 0,
                     (struct sockaddr *)&from, &from_len);
        if (n < 0) {
            printf("Request timeout for icmp_seq %d\n", seq);
            continue;
        }

        struct timeval recv_time;
        gettimeofday(&recv_time, NULL);

        /* Parse: IP header + ICMP reply */
        struct iphdr *ip = (struct iphdr *)recv_buf;
        int ip_hdr_len = ip->ihl * 4;
        struct icmphdr *reply = (struct icmphdr *)(recv_buf + ip_hdr_len);

        if (reply->type == ICMP_ECHOREPLY &&
            ntohs(reply->un.echo.id) == id) {
            received++;
            double rtt = time_diff_ms(&send_time, &recv_time);
            if (rtt < min_rtt) min_rtt = rtt;
            if (rtt > max_rtt) max_rtt = rtt;
            total_rtt += rtt;

            char from_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &from.sin_addr, from_ip, sizeof(from_ip));

            printf("%zd bytes from %s: icmp_seq=%d ttl=%d time=%.1f ms\n",
                   n - ip_hdr_len, from_ip, ntohs(reply->un.echo.sequence),
                   ip->ttl, rtt);
        }

        if (seq < max_count && running)
            sleep(1);
    }

    /* Statistics */
    printf("\n--- %s ping statistics ---\n", target);
    printf("%d packets transmitted, %d received, %.0f%% packet loss\n",
           sent, received,
           sent ? 100.0 * (sent - received) / sent : 0);
    if (received > 0) {
        printf("rtt min/avg/max = %.1f/%.1f/%.1f ms\n",
               min_rtt, total_rtt / received, max_rtt);
    }

    close(fd);
    return 0;
}
