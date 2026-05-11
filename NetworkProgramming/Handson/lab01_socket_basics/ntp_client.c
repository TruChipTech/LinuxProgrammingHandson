/*
 * ntp_client.c — Simple NTP client
 *
 * Build: make
 * Run:   ./ntp_client pool.ntp.org
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>

/* NTP epoch: 1 Jan 1900. Unix epoch: 1 Jan 1970. Difference in seconds. */
#define NTP_EPOCH_OFFSET 2208988800ULL

struct ntp_packet {
    uint8_t  li_vn_mode;
    uint8_t  stratum;
    uint8_t  poll;
    int8_t   precision;
    uint32_t root_delay;
    uint32_t root_dispersion;
    uint32_t ref_id;
    uint32_t ref_ts_sec;
    uint32_t ref_ts_frac;
    uint32_t orig_ts_sec;
    uint32_t orig_ts_frac;
    uint32_t recv_ts_sec;
    uint32_t recv_ts_frac;
    uint32_t xmit_ts_sec;
    uint32_t xmit_ts_frac;
} __attribute__((packed));

int main(int argc, char *argv[])
{
    const char *server = (argc > 1) ? argv[1] : "pool.ntp.org";

    /* Resolve server */
    struct addrinfo hints, *res;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int status = getaddrinfo(server, "123", &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }

    int fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (fd < 0) { perror("socket"); freeaddrinfo(res); return 1; }

    struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    /* Build NTP request */
    struct ntp_packet pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.li_vn_mode = (0 << 6) | (4 << 3) | 3;  /* LI=0, VN=4 (NTPv4), Mode=3 (client) */

    printf("Querying NTP server: %s\n", server);

    if (sendto(fd, &pkt, sizeof(pkt), 0, res->ai_addr, res->ai_addrlen) < 0) {
        perror("sendto");
        close(fd);
        freeaddrinfo(res);
        return 1;
    }

    /* Receive response */
    ssize_t n = recvfrom(fd, &pkt, sizeof(pkt), 0, NULL, NULL);
    if (n < (ssize_t)sizeof(pkt)) {
        fprintf(stderr, "Short/no response (%zd bytes)\n", n);
        close(fd);
        freeaddrinfo(res);
        return 1;
    }

    /* Parse response */
    uint8_t li = (pkt.li_vn_mode >> 6) & 0x3;
    uint8_t vn = (pkt.li_vn_mode >> 3) & 0x7;
    uint8_t mode = pkt.li_vn_mode & 0x7;

    printf("\nNTP Response:\n");
    printf("  LI=%u, Version=%u, Mode=%u\n", li, vn, mode);
    printf("  Stratum: %u", pkt.stratum);
    switch (pkt.stratum) {
        case 0: printf(" (kiss-of-death)\n"); break;
        case 1: printf(" (primary reference)\n"); break;
        default: printf(" (secondary)\n"); break;
    }
    printf("  Poll: %d (2^%d = %d sec)\n", pkt.poll, pkt.poll, 1 << pkt.poll);
    printf("  Precision: %d (2^%d sec)\n", pkt.precision, pkt.precision);

    /* Convert transmit timestamp to local time */
    uint32_t ntp_sec = ntohl(pkt.xmit_ts_sec);
    time_t unix_time = ntp_sec - NTP_EPOCH_OFFSET;
    struct tm *tm = localtime(&unix_time);
    char timebuf[64];
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm);

    printf("\n  NTP Time:   %u (seconds since 1900)\n", ntp_sec);
    printf("  Unix Time:  %ld\n", (long)unix_time);
    printf("  Local Time: %s\n", timebuf);

    /* Compare with local time */
    time_t local_time = time(NULL);
    long diff = (long)(unix_time - local_time);
    printf("\n  Clock offset: %+ld seconds\n", diff);

    close(fd);
    freeaddrinfo(res);
    return 0;
}
