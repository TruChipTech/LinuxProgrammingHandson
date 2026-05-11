/*
 * packet_headers.c — Capture and display IP, TCP, UDP headers from raw packets
 *
 * Build: make
 * Run:   sudo ./packet_headers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

static void print_ip_header(const struct iphdr *ip)
{
    char src[INET_ADDRSTRLEN], dst[INET_ADDRSTRLEN];
    struct in_addr s, d;
    s.s_addr = ip->saddr;
    d.s_addr = ip->daddr;
    inet_ntop(AF_INET, &s, src, sizeof(src));
    inet_ntop(AF_INET, &d, dst, sizeof(dst));

    printf("  IP Header:\n");
    printf("    Version:  %u\n", ip->version);
    printf("    IHL:      %u (%u bytes)\n", ip->ihl, ip->ihl * 4);
    printf("    TOS:      0x%02x\n", ip->tos);
    printf("    Total Len:%u\n", ntohs(ip->tot_len));
    printf("    ID:       %u\n", ntohs(ip->id));
    printf("    TTL:      %u\n", ip->ttl);
    printf("    Protocol: %u", ip->protocol);
    switch (ip->protocol) {
        case IPPROTO_TCP:  printf(" (TCP)\n"); break;
        case IPPROTO_UDP:  printf(" (UDP)\n"); break;
        case IPPROTO_ICMP: printf(" (ICMP)\n"); break;
        default:           printf("\n"); break;
    }
    printf("    Checksum: 0x%04x\n", ntohs(ip->check));
    printf("    Src:      %s\n", src);
    printf("    Dst:      %s\n", dst);
}

static void print_tcp_header(const struct tcphdr *tcp)
{
    printf("  TCP Header:\n");
    printf("    Src Port:  %u\n", ntohs(tcp->source));
    printf("    Dst Port:  %u\n", ntohs(tcp->dest));
    printf("    Seq:       %u\n", ntohl(tcp->seq));
    printf("    Ack:       %u\n", ntohl(tcp->ack_seq));
    printf("    Data Off:  %u (%u bytes)\n", tcp->doff, tcp->doff * 4);
    printf("    Flags:     ");
    if (tcp->syn) printf("SYN ");
    if (tcp->ack) printf("ACK ");
    if (tcp->fin) printf("FIN ");
    if (tcp->rst) printf("RST ");
    if (tcp->psh) printf("PSH ");
    if (tcp->urg) printf("URG ");
    printf("\n");
    printf("    Window:    %u\n", ntohs(tcp->window));
    printf("    Checksum:  0x%04x\n", ntohs(tcp->check));
}

static void print_udp_header(const struct udphdr *udp)
{
    printf("  UDP Header:\n");
    printf("    Src Port:  %u\n", ntohs(udp->source));
    printf("    Dst Port:  %u\n", ntohs(udp->dest));
    printf("    Length:    %u\n", ntohs(udp->len));
    printf("    Checksum:  0x%04x\n", ntohs(udp->check));
}

static void print_icmp_header(const struct icmphdr *icmp)
{
    printf("  ICMP Header:\n");
    printf("    Type:      %u", icmp->type);
    switch (icmp->type) {
        case ICMP_ECHOREPLY:    printf(" (Echo Reply)\n"); break;
        case ICMP_ECHO:         printf(" (Echo Request)\n"); break;
        case ICMP_DEST_UNREACH: printf(" (Dest Unreachable)\n"); break;
        case ICMP_TIME_EXCEEDED:printf(" (Time Exceeded)\n"); break;
        default:                printf("\n"); break;
    }
    printf("    Code:      %u\n", icmp->code);
    printf("    Checksum:  0x%04x\n", ntohs(icmp->checksum));
}

int main(void)
{
    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (fd < 0) {
        perror("socket (need root)");
        return 1;
    }

    /* Also open UDP and ICMP raw sockets */
    int fd_udp = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    int fd_icmp = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    printf("Capturing packets... (Ctrl+C to stop)\n");
    printf("Generate traffic: ping, curl, etc.\n\n");

    unsigned char buf[65536];
    int count = 0;

    while (count < 20) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        int maxfd = fd;
        if (fd_udp >= 0) { FD_SET(fd_udp, &fds); if (fd_udp > maxfd) maxfd = fd_udp; }
        if (fd_icmp >= 0) { FD_SET(fd_icmp, &fds); if (fd_icmp > maxfd) maxfd = fd_icmp; }

        struct timeval tv = { .tv_sec = 10, .tv_usec = 0 };
        int ret = select(maxfd + 1, &fds, NULL, NULL, &tv);
        if (ret <= 0) break;

        int recv_fd = -1;
        if (FD_ISSET(fd, &fds)) recv_fd = fd;
        else if (fd_udp >= 0 && FD_ISSET(fd_udp, &fds)) recv_fd = fd_udp;
        else if (fd_icmp >= 0 && FD_ISSET(fd_icmp, &fds)) recv_fd = fd_icmp;

        if (recv_fd < 0) continue;

        ssize_t n = recvfrom(recv_fd, buf, sizeof(buf), 0, NULL, NULL);
        if (n < (ssize_t)sizeof(struct iphdr)) continue;

        count++;
        printf("--- Packet #%d (%zd bytes) ---\n", count, n);

        struct iphdr *ip = (struct iphdr *)buf;
        print_ip_header(ip);

        int ip_hdr_len = ip->ihl * 4;
        unsigned char *payload = buf + ip_hdr_len;

        switch (ip->protocol) {
        case IPPROTO_TCP:
            if (n >= ip_hdr_len + (int)sizeof(struct tcphdr))
                print_tcp_header((struct tcphdr *)payload);
            break;
        case IPPROTO_UDP:
            if (n >= ip_hdr_len + (int)sizeof(struct udphdr))
                print_udp_header((struct udphdr *)payload);
            break;
        case IPPROTO_ICMP:
            if (n >= ip_hdr_len + (int)sizeof(struct icmphdr))
                print_icmp_header((struct icmphdr *)payload);
            break;
        }
        printf("\n");
    }

    close(fd);
    if (fd_udp >= 0) close(fd_udp);
    if (fd_icmp >= 0) close(fd_icmp);
    return 0;
}
