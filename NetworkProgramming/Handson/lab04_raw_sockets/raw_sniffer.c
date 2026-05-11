/*
 * raw_sniffer.c — Packet sniffer using raw sockets
 *
 * Build: make
 * Run:   sudo ./raw_sniffer
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
#include <linux/if_ether.h>
#include <signal.h>

static volatile int running = 1;
static void sigint_handler(int sig) { (void)sig; running = 0; }

static unsigned long pkt_count = 0;
static unsigned long tcp_count = 0;
static unsigned long udp_count = 0;
static unsigned long icmp_count = 0;
static unsigned long other_count = 0;

static void process_packet(unsigned char *buf, ssize_t len)
{
    if (len < (ssize_t)sizeof(struct iphdr)) return;

    struct iphdr *ip = (struct iphdr *)buf;
    int ip_hdr_len = ip->ihl * 4;

    char src[INET_ADDRSTRLEN], dst[INET_ADDRSTRLEN];
    struct in_addr s, d;
    s.s_addr = ip->saddr;
    d.s_addr = ip->daddr;
    inet_ntop(AF_INET, &s, src, sizeof(src));
    inet_ntop(AF_INET, &d, dst, sizeof(dst));

    pkt_count++;

    switch (ip->protocol) {
    case IPPROTO_TCP: {
        tcp_count++;
        if (len >= ip_hdr_len + (int)sizeof(struct tcphdr)) {
            struct tcphdr *tcp = (struct tcphdr *)(buf + ip_hdr_len);
            printf("[TCP] %s:%d → %s:%d  len=%zd  ",
                   src, ntohs(tcp->source), dst, ntohs(tcp->dest), len);
            if (tcp->syn) printf("SYN ");
            if (tcp->ack) printf("ACK ");
            if (tcp->fin) printf("FIN ");
            if (tcp->rst) printf("RST ");
            if (tcp->psh) printf("PSH ");
            printf("\n");
        }
        break;
    }
    case IPPROTO_UDP: {
        udp_count++;
        if (len >= ip_hdr_len + (int)sizeof(struct udphdr)) {
            struct udphdr *udp = (struct udphdr *)(buf + ip_hdr_len);
            printf("[UDP] %s:%d → %s:%d  len=%zd\n",
                   src, ntohs(udp->source), dst, ntohs(udp->dest), len);
        }
        break;
    }
    case IPPROTO_ICMP: {
        icmp_count++;
        if (len >= ip_hdr_len + (int)sizeof(struct icmphdr)) {
            struct icmphdr *icmp = (struct icmphdr *)(buf + ip_hdr_len);
            const char *type_str = "Other";
            if (icmp->type == ICMP_ECHO) type_str = "Echo Request";
            else if (icmp->type == ICMP_ECHOREPLY) type_str = "Echo Reply";
            else if (icmp->type == ICMP_DEST_UNREACH) type_str = "Dest Unreachable";
            printf("[ICMP] %s → %s  type=%s(%d) code=%d\n",
                   src, dst, type_str, icmp->type, icmp->code);
        }
        break;
    }
    default:
        other_count++;
        printf("[Proto %d] %s → %s  len=%zd\n", ip->protocol, src, dst, len);
        break;
    }
}

int main(void)
{
    /* Create raw socket to capture all IP packets */
    int fd = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (fd < 0) {
        perror("socket (need root/CAP_NET_RAW)");
        return 1;
    }

    /* Also listen for UDP and ICMP */
    int fd_udp = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    int fd_icmp = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);

    signal(SIGINT, sigint_handler);

    printf("Raw Socket Packet Sniffer (Ctrl+C to stop)\n");
    printf("Generate traffic: ping, curl, etc.\n\n");

    unsigned char buf[65536];

    while (running) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);
        int maxfd = fd;
        if (fd_udp >= 0) { FD_SET(fd_udp, &fds); if (fd_udp > maxfd) maxfd = fd_udp; }
        if (fd_icmp >= 0) { FD_SET(fd_icmp, &fds); if (fd_icmp > maxfd) maxfd = fd_icmp; }

        struct timeval tv = { .tv_sec = 1, .tv_usec = 0 };
        int ret = select(maxfd + 1, &fds, NULL, NULL, &tv);
        if (ret <= 0) continue;

        int recv_fd = -1;
        if (FD_ISSET(fd, &fds)) recv_fd = fd;
        else if (fd_udp >= 0 && FD_ISSET(fd_udp, &fds)) recv_fd = fd_udp;
        else if (fd_icmp >= 0 && FD_ISSET(fd_icmp, &fds)) recv_fd = fd_icmp;
        if (recv_fd < 0) continue;

        ssize_t n = recvfrom(recv_fd, buf, sizeof(buf), 0, NULL, NULL);
        if (n > 0)
            process_packet(buf, n);
    }

    printf("\n=== Statistics ===\n");
    printf("Total: %lu  TCP: %lu  UDP: %lu  ICMP: %lu  Other: %lu\n",
           pkt_count, tcp_count, udp_count, icmp_count, other_count);

    close(fd);
    if (fd_udp >= 0) close(fd_udp);
    if (fd_icmp >= 0) close(fd_icmp);
    return 0;
}
