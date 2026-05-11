/*
 * dns_lookup.c — Hostname resolution using getaddrinfo / getnameinfo
 *
 * Build: make
 * Run:   ./dns_lookup example.com
 *        ./dns_lookup google.com
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <hostname> [service]\n", argv[0]);
        return 1;
    }

    const char *host = argv[1];
    const char *service = (argc > 2) ? argv[2] : NULL;

    struct addrinfo hints, *res, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;     /* IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* TCP */

    printf("Resolving: %s", host);
    if (service) printf(" (service: %s)", service);
    printf("\n\n");

    int status = getaddrinfo(host, service, &hints, &res);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 2;
    }

    int count = 0;
    for (p = res; p != NULL; p = p->ai_next) {
        count++;
        char ipstr[INET6_ADDRSTRLEN];
        void *addr;
        const char *family;
        int port = 0;

        if (p->ai_family == AF_INET) {
            struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
            addr = &ipv4->sin_addr;
            port = ntohs(ipv4->sin_port);
            family = "IPv4";
        } else {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
            addr = &ipv6->sin6_addr;
            port = ntohs(ipv6->sin6_port);
            family = "IPv6";
        }

        inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
        printf("  [%d] %s: %s", count, family, ipstr);
        if (port) printf(" port %d", port);

        /* Socket type */
        switch (p->ai_socktype) {
        case SOCK_STREAM: printf(" (TCP)"); break;
        case SOCK_DGRAM:  printf(" (UDP)"); break;
        case SOCK_RAW:    printf(" (RAW)"); break;
        }
        printf("\n");

        /* Reverse lookup */
        char revhost[NI_MAXHOST], revserv[NI_MAXSERV];
        if (getnameinfo(p->ai_addr, p->ai_addrlen,
                        revhost, sizeof(revhost),
                        revserv, sizeof(revserv), 0) == 0) {
            printf("       Reverse: %s", revhost);
            if (revserv[0]) printf(" (service: %s)", revserv);
            printf("\n");
        }
    }

    printf("\nTotal addresses: %d\n", count);
    freeaddrinfo(res);
    return 0;
}
