/*
 * socket_info.c — Socket fundamentals: creation, address structures, byte order
 *
 * Demonstrates:
 *  - socket() with different types
 *  - struct sockaddr_in / sockaddr_in6 layout and sizes
 *  - htons/htonl/ntohs/ntohl byte order conversion
 *  - inet_pton / inet_ntop address conversion
 *
 * Build: make
 * Run:   ./socket_info
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

static void demo_socket_types(void)
{
    int fd;

    printf("=== Socket Types ===\n\n");

    /* TCP socket */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd >= 0) {
        printf("TCP socket (SOCK_STREAM): fd=%d\n", fd);
        close(fd);
    } else {
        perror("TCP socket");
    }

    /* UDP socket */
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd >= 0) {
        printf("UDP socket (SOCK_DGRAM):  fd=%d\n", fd);
        close(fd);
    } else {
        perror("UDP socket");
    }

    /* Raw socket (needs root) */
    fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (fd >= 0) {
        printf("Raw socket (SOCK_RAW):    fd=%d\n", fd);
        close(fd);
    } else {
        printf("Raw socket (SOCK_RAW):    %s (need root)\n", strerror(errno));
    }

    /* IPv6 TCP socket */
    fd = socket(AF_INET6, SOCK_STREAM, 0);
    if (fd >= 0) {
        printf("IPv6 TCP socket:          fd=%d\n", fd);
        close(fd);
    } else {
        perror("IPv6 TCP");
    }

    printf("\n");
}

static void demo_address_structures(void)
{
    printf("=== Address Structure Sizes ===\n\n");
    printf("sizeof(struct sockaddr)     = %zu bytes\n", sizeof(struct sockaddr));
    printf("sizeof(struct sockaddr_in)  = %zu bytes (IPv4)\n", sizeof(struct sockaddr_in));
    printf("sizeof(struct sockaddr_in6) = %zu bytes (IPv6)\n", sizeof(struct sockaddr_in6));
    printf("\n");

    /* Fill an IPv4 address */
    struct sockaddr_in addr4;
    memset(&addr4, 0, sizeof(addr4));
    addr4.sin_family = AF_INET;
    addr4.sin_port = htons(8080);
    inet_pton(AF_INET, "192.168.1.100", &addr4.sin_addr);

    char ip4str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr4.sin_addr, ip4str, sizeof(ip4str));
    printf("IPv4 address: %s, port: %d\n", ip4str, ntohs(addr4.sin_port));

    /* Fill an IPv6 address */
    struct sockaddr_in6 addr6;
    memset(&addr6, 0, sizeof(addr6));
    addr6.sin6_family = AF_INET6;
    addr6.sin6_port = htons(8080);
    inet_pton(AF_INET6, "2001:db8::1", &addr6.sin6_addr);

    char ip6str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &addr6.sin6_addr, ip6str, sizeof(ip6str));
    printf("IPv6 address: %s, port: %d\n", ip6str, ntohs(addr6.sin6_port));
    printf("\n");
}

static void demo_byte_order(void)
{
    printf("=== Byte Order Conversion ===\n\n");

    uint16_t port = 8080;
    printf("Port %u:\n", port);
    printf("  Host order:    0x%04x\n", port);
    printf("  Network order: 0x%04x  (htons)\n", htons(port));
    printf("  Back to host:  0x%04x  (ntohs)\n", ntohs(htons(port)));

    uint32_t addr = 0xC0A80164; /* 192.168.1.100 */
    printf("\nAddress 192.168.1.100:\n");
    printf("  Host order:    0x%08x\n", addr);
    printf("  Network order: 0x%08x  (htonl)\n", htonl(addr));

    /* Check endianness */
    uint16_t test = 0x0102;
    unsigned char *p = (unsigned char *)&test;
    printf("\nSystem endianness: %s\n",
           (p[0] == 0x01) ? "BIG-ENDIAN" : "LITTLE-ENDIAN");
    printf("\n");
}

static void demo_address_conversion(void)
{
    printf("=== Address Conversion (inet_pton / inet_ntop) ===\n\n");

    /* String → binary */
    struct in_addr ipv4;
    const char *ip4 = "10.0.0.1";
    if (inet_pton(AF_INET, ip4, &ipv4) == 1) {
        printf("inet_pton(\"%s\") → 0x%08x\n", ip4, ntohl(ipv4.s_addr));
    }

    /* Binary → string */
    char buf[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ipv4, buf, sizeof(buf));
    printf("inet_ntop(0x%08x) → \"%s\"\n", ntohl(ipv4.s_addr), buf);

    /* IPv6 */
    struct in6_addr ipv6;
    const char *ip6 = "2001:db8::1";
    if (inet_pton(AF_INET6, ip6, &ipv6) == 1) {
        char buf6[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &ipv6, buf6, sizeof(buf6));
        printf("IPv6: \"%s\" → binary → \"%s\"\n", ip6, buf6);
    }

    /* Special addresses */
    printf("\nSpecial addresses:\n");
    printf("  INADDR_ANY       = %s\n", "0.0.0.0 (bind to all interfaces)");
    printf("  INADDR_LOOPBACK  = %s\n", "127.0.0.1 (loopback)");
    printf("  INADDR_BROADCAST = %s\n", "255.255.255.255 (broadcast)");
    printf("\n");
}

int main(void)
{
    printf("============================================\n");
    printf("  Socket Fundamentals Demo\n");
    printf("============================================\n\n");

    demo_socket_types();
    demo_address_structures();
    demo_byte_order();
    demo_address_conversion();

    return 0;
}
