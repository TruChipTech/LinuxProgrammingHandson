/*
 * send_custom_packet.c — Send a packet with custom EtherType (user-space companion)
 *
 * Sends a raw Ethernet frame with EtherType 0x88B5 to test the custom_protocol
 * kernel module.
 *
 * Build: make
 * Run:   sudo ./send_custom_packet [interface]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <arpa/inet.h>

#define CUSTOM_ETH_TYPE 0x88B5
#define CUSTOM_MAGIC    0xDEADBEEF

struct custom_header {
    uint32_t magic;
    uint16_t version;
    uint16_t msg_type;
    uint32_t payload_len;
} __attribute__((packed));

int main(int argc, char *argv[])
{
    const char *ifname = (argc > 1) ? argv[1] : "lo";

    /* Create raw socket */
    int fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (fd < 0) {
        perror("socket (need root)");
        return 1;
    }

    /* Get interface index and MAC */
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, ifname, IFNAMSIZ - 1);

    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        perror("SIOCGIFINDEX");
        close(fd);
        return 1;
    }
    int ifindex = ifr.ifr_ifindex;

    if (ioctl(fd, SIOCGIFHWADDR, &ifr) < 0) {
        perror("SIOCGIFHWADDR");
        close(fd);
        return 1;
    }
    unsigned char *src_mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;

    /* Build frame */
    unsigned char frame[128];
    int pos = 0;

    /* Ethernet header */
    /* Destination: broadcast (or loopback MAC for lo) */
    unsigned char dst_mac[6];
    if (strcmp(ifname, "lo") == 0) {
        memcpy(dst_mac, src_mac, 6);
    } else {
        memset(dst_mac, 0xff, 6); /* broadcast */
    }
    memcpy(frame + pos, dst_mac, 6); pos += 6;
    memcpy(frame + pos, src_mac, 6); pos += 6;
    uint16_t ethtype = htons(CUSTOM_ETH_TYPE);
    memcpy(frame + pos, &ethtype, 2); pos += 2;

    /* Custom header */
    struct custom_header hdr;
    hdr.magic = htonl(CUSTOM_MAGIC);
    hdr.version = htons(1);
    hdr.msg_type = htons(1);  /* HELLO */

    const char *payload = "Hello from custom protocol!";
    hdr.payload_len = htonl(strlen(payload));

    memcpy(frame + pos, &hdr, sizeof(hdr)); pos += sizeof(hdr);

    /* Payload */
    memcpy(frame + pos, payload, strlen(payload)); pos += strlen(payload);

    /* Pad to minimum Ethernet frame size (60 bytes without FCS) */
    while (pos < 60) frame[pos++] = 0;

    /* Send */
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_ifindex = ifindex;
    sll.sll_halen = 6;
    memcpy(sll.sll_addr, dst_mac, 6);

    printf("Sending custom packet on %s (EtherType 0x%04x, %d bytes)...\n",
           ifname, CUSTOM_ETH_TYPE, pos);

    ssize_t sent = sendto(fd, frame, pos, 0,
                          (struct sockaddr *)&sll, sizeof(sll));
    if (sent < 0) {
        perror("sendto");
    } else {
        printf("Sent %zd bytes. Check: dmesg | tail\n", sent);
    }

    close(fd);
    return 0;
}
