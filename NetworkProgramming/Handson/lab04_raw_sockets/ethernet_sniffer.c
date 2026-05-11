/*
 * ethernet_sniffer.c — Layer 2 Ethernet frame capture using AF_PACKET
 *
 * Build: make
 * Run:   sudo ./ethernet_sniffer
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <linux/if_ether.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>

static volatile int running = 1;
static void sigint_handler(int sig) { (void)sig; running = 0; }

static void print_mac(const unsigned char *mac)
{
    printf("%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static const char *ethertype_str(uint16_t proto)
{
    switch (proto) {
    case ETH_P_IP:    return "IPv4";
    case ETH_P_IPV6:  return "IPv6";
    case ETH_P_ARP:   return "ARP";
    case ETH_P_8021Q: return "VLAN";
    case ETH_P_LOOP:  return "Loopback";
    default:          return "Other";
    }
}

int main(int argc, char *argv[])
{
    /* Create AF_PACKET raw socket */
    int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0) {
        perror("socket (need root/CAP_NET_RAW)");
        return 1;
    }

    /* Optionally bind to specific interface */
    if (argc > 1) {
        struct sockaddr_ll sll;
        memset(&sll, 0, sizeof(sll));
        sll.sll_family = AF_PACKET;
        sll.sll_ifindex = if_nametoindex(argv[1]);
        sll.sll_protocol = htons(ETH_P_ALL);
        if (sll.sll_ifindex == 0) {
            fprintf(stderr, "Unknown interface: %s\n", argv[1]);
            close(fd);
            return 1;
        }
        if (bind(fd, (struct sockaddr *)&sll, sizeof(sll)) < 0) {
            perror("bind");
            close(fd);
            return 1;
        }
        printf("Bound to interface: %s\n", argv[1]);
    }

    signal(SIGINT, sigint_handler);
    printf("Ethernet Frame Sniffer (Ctrl+C to stop)\n\n");

    unsigned char buf[65536];
    int count = 0;

    while (running && count < 50) {
        ssize_t n = recvfrom(fd, buf, sizeof(buf), 0, NULL, NULL);
        if (n < (ssize_t)sizeof(struct ethhdr)) continue;

        count++;
        struct ethhdr *eth = (struct ethhdr *)buf;
        uint16_t proto = ntohs(eth->h_proto);

        printf("#%d [%zd bytes] ", count, n);
        print_mac(eth->h_source);
        printf(" → ");
        print_mac(eth->h_dest);
        printf("  EtherType=0x%04x (%s)", proto, ethertype_str(proto));

        /* If IP, show IP addresses */
        if (proto == ETH_P_IP && n >= (ssize_t)(sizeof(struct ethhdr) + sizeof(struct iphdr))) {
            struct iphdr *ip = (struct iphdr *)(buf + sizeof(struct ethhdr));
            char src[INET_ADDRSTRLEN], dst[INET_ADDRSTRLEN];
            struct in_addr s, d;
            s.s_addr = ip->saddr;
            d.s_addr = ip->daddr;
            inet_ntop(AF_INET, &s, src, sizeof(src));
            inet_ntop(AF_INET, &d, dst, sizeof(dst));
            printf("  %s → %s", src, dst);
        }
        printf("\n");
    }

    printf("\nCaptured %d frames.\n", count);
    close(fd);
    return 0;
}
