/*
 * bpf_filter.c — Classic BPF socket filter: capture only ICMP packets
 *
 * Build: make
 * Run:   sudo ./bpf_filter
 * Test:  ping -c 3 127.0.0.1 (in another terminal)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <linux/filter.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>

static volatile int running = 1;
static void sigint_handler(int sig) { (void)sig; running = 0; }

int main(void)
{
    /* Create raw socket to receive all Ethernet frames */
    int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0) {
        perror("socket (need root)");
        return 1;
    }

    /*
     * BPF filter program: accept only ICMP packets
     *
     * Pseudocode:
     *   if (EtherType != 0x0800) goto reject
     *   if (IP.protocol != 1 [ICMP]) goto reject
     *   accept(65535)
     *   reject(0)
     */
    struct sock_filter code[] = {
        /* Load EtherType (offset 12 in Ethernet frame) */
        BPF_STMT(BPF_LD | BPF_H | BPF_ABS, 12),
        /* If not IP (0x0800), skip to reject */
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, ETH_P_IP, 0, 3),
        /* Load IP protocol field (offset 23 = 14 [eth] + 9 [ip.protocol]) */
        BPF_STMT(BPF_LD | BPF_B | BPF_ABS, 23),
        /* If ICMP (1), accept */
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, IPPROTO_ICMP, 0, 1),
        /* Accept: return 65535 (capture entire packet) */
        BPF_STMT(BPF_RET | BPF_K, 65535),
        /* Reject: return 0 (drop packet) */
        BPF_STMT(BPF_RET | BPF_K, 0),
    };

    struct sock_fprog bpf = {
        .len = sizeof(code) / sizeof(code[0]),
        .filter = code,
    };

    /* Attach BPF filter to socket */
    if (setsockopt(fd, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf)) < 0) {
        perror("setsockopt SO_ATTACH_FILTER");
        close(fd);
        return 1;
    }

    signal(SIGINT, sigint_handler);
    printf("BPF ICMP Filter active (Ctrl+C to stop)\n");
    printf("Only ICMP packets will be shown.\n");
    printf("Test: ping -c 3 127.0.0.1\n\n");

    unsigned char buf[65536];
    int count = 0;

    while (running) {
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n < 0) {
            if (!running) break;
            continue;
        }

        /* Skip Ethernet header (14 bytes) */
        if (n < 14 + (int)sizeof(struct iphdr)) continue;

        struct iphdr *ip = (struct iphdr *)(buf + 14);
        int ip_hdr_len = ip->ihl * 4;

        if (n < 14 + ip_hdr_len + (int)sizeof(struct icmphdr)) continue;

        struct icmphdr *icmp = (struct icmphdr *)(buf + 14 + ip_hdr_len);

        char src[INET_ADDRSTRLEN], dst[INET_ADDRSTRLEN];
        struct in_addr s, d;
        s.s_addr = ip->saddr;
        d.s_addr = ip->daddr;
        inet_ntop(AF_INET, &s, src, sizeof(src));
        inet_ntop(AF_INET, &d, dst, sizeof(dst));

        count++;
        const char *type_str = "?";
        if (icmp->type == ICMP_ECHO) type_str = "Echo Request";
        else if (icmp->type == ICMP_ECHOREPLY) type_str = "Echo Reply";
        else if (icmp->type == ICMP_DEST_UNREACH) type_str = "Unreachable";

        printf("[%d] ICMP %s → %s  type=%s(%d) code=%d  id=%u seq=%u\n",
               count, src, dst, type_str, icmp->type, icmp->code,
               ntohs(icmp->un.echo.id), ntohs(icmp->un.echo.sequence));
    }

    printf("\nCaptured %d ICMP packets.\n", count);
    close(fd);
    return 0;
}
