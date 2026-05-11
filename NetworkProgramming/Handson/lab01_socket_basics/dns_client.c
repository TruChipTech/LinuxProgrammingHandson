/*
 * dns_client.c — Build and parse DNS query packets from scratch
 *
 * Build: make
 * Run:   ./dns_client example.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

/* DNS header */
struct dns_header {
    uint16_t id;
    uint16_t flags;
    uint16_t qdcount;
    uint16_t ancount;
    uint16_t nscount;
    uint16_t arcount;
} __attribute__((packed));

/* DNS question fields (after QNAME) */
struct dns_question {
    uint16_t qtype;
    uint16_t qclass;
} __attribute__((packed));

/* DNS resource record fields (after NAME) */
struct dns_rr {
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t rdlength;
} __attribute__((packed));

/* Encode domain name: "example.com" → "\7example\3com\0" */
static int encode_name(const char *name, unsigned char *buf, int bufsize)
{
    int pos = 0;
    const char *p = name;

    while (*p) {
        const char *dot = strchr(p, '.');
        int len = dot ? (int)(dot - p) : (int)strlen(p);

        if (pos + 1 + len >= bufsize) return -1;
        buf[pos++] = (unsigned char)len;
        memcpy(buf + pos, p, len);
        pos += len;

        p += len;
        if (*p == '.') p++;
    }
    if (pos >= bufsize) return -1;
    buf[pos++] = 0; /* Root label */
    return pos;
}

/* Parse a DNS name from response (handles pointer compression) */
static int parse_name(const unsigned char *pkt, int pktlen, int offset,
                      char *out, int outsize)
{
    int pos = offset;
    int out_pos = 0;
    int jumped = 0;
    int saved_pos = 0;

    while (pos < pktlen) {
        unsigned char len = pkt[pos];

        if (len == 0) {
            pos++;
            break;
        }

        /* Pointer (compression) */
        if ((len & 0xC0) == 0xC0) {
            if (!jumped) saved_pos = pos + 2;
            pos = ((len & 0x3F) << 8) | pkt[pos + 1];
            jumped = 1;
            continue;
        }

        pos++;
        if (out_pos > 0 && out_pos < outsize - 1) out[out_pos++] = '.';
        for (int i = 0; i < len && pos < pktlen && out_pos < outsize - 1; i++)
            out[out_pos++] = pkt[pos++];
    }

    out[out_pos] = '\0';
    return jumped ? saved_pos : pos;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <domain>\n", argv[0]);
        return 1;
    }

    const char *domain = argv[1];
    unsigned char packet[512];
    int pkt_pos = 0;

    /* Build DNS header */
    struct dns_header *hdr = (struct dns_header *)packet;
    srand(time(NULL));
    hdr->id = htons(rand() & 0xFFFF);
    hdr->flags = htons(0x0100);  /* Standard query, recursion desired */
    hdr->qdcount = htons(1);
    hdr->ancount = 0;
    hdr->nscount = 0;
    hdr->arcount = 0;
    pkt_pos = sizeof(struct dns_header);

    /* Encode question name */
    int name_len = encode_name(domain, packet + pkt_pos, sizeof(packet) - pkt_pos);
    if (name_len < 0) {
        fprintf(stderr, "Domain too long\n");
        return 1;
    }
    pkt_pos += name_len;

    /* Question type and class */
    struct dns_question *q = (struct dns_question *)(packet + pkt_pos);
    q->qtype = htons(1);   /* A record */
    q->qclass = htons(1);  /* IN (Internet) */
    pkt_pos += sizeof(struct dns_question);

    /* Send to DNS server (8.8.8.8:53 via UDP) */
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); return 1; }

    struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in dns_server;
    memset(&dns_server, 0, sizeof(dns_server));
    dns_server.sin_family = AF_INET;
    dns_server.sin_port = htons(53);
    inet_pton(AF_INET, "8.8.8.8", &dns_server.sin_addr);

    printf("Querying 8.8.8.8 for %s (A record)...\n", domain);

    ssize_t sent = sendto(fd, packet, pkt_pos, 0,
                          (struct sockaddr *)&dns_server, sizeof(dns_server));
    if (sent < 0) { perror("sendto"); close(fd); return 1; }

    /* Receive response */
    unsigned char resp[512];
    ssize_t n = recvfrom(fd, resp, sizeof(resp), 0, NULL, NULL);
    if (n < 0) { perror("recvfrom (timeout?)"); close(fd); return 1; }

    printf("Received %zd bytes\n\n", n);

    /* Parse response header */
    struct dns_header *rhdr = (struct dns_header *)resp;
    int rcode = ntohs(rhdr->flags) & 0x000F;
    int ancount = ntohs(rhdr->ancount);

    printf("Response: ID=0x%04x, RCODE=%d, Answers=%d\n",
           ntohs(rhdr->id), rcode, ancount);

    if (rcode != 0) {
        printf("DNS error (RCODE=%d)\n", rcode);
        close(fd);
        return 1;
    }

    /* Skip question section */
    int pos = sizeof(struct dns_header);
    char name[256];
    pos = parse_name(resp, n, pos, name, sizeof(name));
    pos += sizeof(struct dns_question);

    /* Parse answers */
    printf("\nAnswers:\n");
    for (int i = 0; i < ancount && pos < n; i++) {
        pos = parse_name(resp, n, pos, name, sizeof(name));

        if (pos + (int)sizeof(struct dns_rr) > n) break;
        struct dns_rr *rr = (struct dns_rr *)(resp + pos);
        pos += sizeof(struct dns_rr);

        uint16_t rtype = ntohs(rr->type);
        uint16_t rdlen = ntohs(rr->rdlength);
        uint32_t ttl = ntohl(rr->ttl);

        printf("  %s: ", name);

        if (rtype == 1 && rdlen == 4) { /* A record */
            char ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, resp + pos, ip, sizeof(ip));
            printf("A %s (TTL=%u)\n", ip, ttl);
        } else if (rtype == 28 && rdlen == 16) { /* AAAA record */
            char ip[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, resp + pos, ip, sizeof(ip));
            printf("AAAA %s (TTL=%u)\n", ip, ttl);
        } else if (rtype == 5) { /* CNAME */
            char cname[256];
            parse_name(resp, n, pos, cname, sizeof(cname));
            printf("CNAME %s (TTL=%u)\n", cname, ttl);
        } else {
            printf("type=%u rdlen=%u (TTL=%u)\n", rtype, rdlen, ttl);
        }
        pos += rdlen;
    }

    close(fd);
    return 0;
}
