/*
 * socket_options.c — Demonstrate common socket options
 *
 * Build: make
 * Run:   ./socket_options
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

static void print_opt_int(int fd, int level, int optname, const char *name)
{
    int val;
    socklen_t len = sizeof(val);
    if (getsockopt(fd, level, optname, &val, &len) == 0)
        printf("  %-20s = %d\n", name, val);
    else
        printf("  %-20s = (error)\n", name);
}

static void print_opt_timeval(int fd, int level, int optname, const char *name)
{
    struct timeval tv;
    socklen_t len = sizeof(tv);
    if (getsockopt(fd, level, optname, &tv, &len) == 0)
        printf("  %-20s = %ld.%06ld sec\n", name, (long)tv.tv_sec, (long)tv.tv_usec);
    else
        printf("  %-20s = (error)\n", name);
}

int main(void)
{
    int tcp_fd, udp_fd;

    tcp_fd = socket(AF_INET, SOCK_STREAM, 0);
    udp_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (tcp_fd < 0 || udp_fd < 0) {
        perror("socket");
        return 1;
    }

    printf("=== TCP Socket Options (defaults) ===\n");
    print_opt_int(tcp_fd, SOL_SOCKET, SO_REUSEADDR,  "SO_REUSEADDR");
    print_opt_int(tcp_fd, SOL_SOCKET, SO_KEEPALIVE,   "SO_KEEPALIVE");
    print_opt_int(tcp_fd, SOL_SOCKET, SO_RCVBUF,      "SO_RCVBUF");
    print_opt_int(tcp_fd, SOL_SOCKET, SO_SNDBUF,      "SO_SNDBUF");
    print_opt_int(tcp_fd, SOL_SOCKET, SO_ERROR,       "SO_ERROR");
    print_opt_int(tcp_fd, SOL_SOCKET, SO_TYPE,        "SO_TYPE");
    print_opt_timeval(tcp_fd, SOL_SOCKET, SO_RCVTIMEO, "SO_RCVTIMEO");
    print_opt_timeval(tcp_fd, SOL_SOCKET, SO_SNDTIMEO, "SO_SNDTIMEO");
    print_opt_int(tcp_fd, IPPROTO_TCP, TCP_NODELAY,   "TCP_NODELAY");
    print_opt_int(tcp_fd, IPPROTO_IP,  IP_TTL,        "IP_TTL");

    printf("\n=== Setting options ===\n");

    /* SO_REUSEADDR */
    int opt = 1;
    setsockopt(tcp_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    print_opt_int(tcp_fd, SOL_SOCKET, SO_REUSEADDR, "SO_REUSEADDR");

    /* SO_RCVBUF */
    int bufsize = 65536;
    setsockopt(tcp_fd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
    print_opt_int(tcp_fd, SOL_SOCKET, SO_RCVBUF, "SO_RCVBUF");

    /* SO_RCVTIMEO */
    struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
    setsockopt(tcp_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    print_opt_timeval(tcp_fd, SOL_SOCKET, SO_RCVTIMEO, "SO_RCVTIMEO");

    /* TCP_NODELAY */
    opt = 1;
    setsockopt(tcp_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
    print_opt_int(tcp_fd, IPPROTO_TCP, TCP_NODELAY, "TCP_NODELAY");

    /* SO_KEEPALIVE */
    opt = 1;
    setsockopt(tcp_fd, SOL_SOCKET, SO_KEEPALIVE, &opt, sizeof(opt));
    print_opt_int(tcp_fd, SOL_SOCKET, SO_KEEPALIVE, "SO_KEEPALIVE");

    printf("\n=== UDP Socket Options ===\n");
    print_opt_int(udp_fd, SOL_SOCKET, SO_BROADCAST, "SO_BROADCAST");
    opt = 1;
    setsockopt(udp_fd, SOL_SOCKET, SO_BROADCAST, &opt, sizeof(opt));
    print_opt_int(udp_fd, SOL_SOCKET, SO_BROADCAST, "SO_BROADCAST (set)");

    printf("\n=== SO_LINGER ===\n");
    struct linger lg;
    socklen_t lglen = sizeof(lg);
    getsockopt(tcp_fd, SOL_SOCKET, SO_LINGER, &lg, &lglen);
    printf("  Default: onoff=%d, linger=%d\n", lg.l_onoff, lg.l_linger);
    lg.l_onoff = 1;
    lg.l_linger = 5;
    setsockopt(tcp_fd, SOL_SOCKET, SO_LINGER, &lg, sizeof(lg));
    getsockopt(tcp_fd, SOL_SOCKET, SO_LINGER, &lg, &lglen);
    printf("  After set: onoff=%d, linger=%d sec\n", lg.l_onoff, lg.l_linger);

    close(tcp_fd);
    close(udp_fd);
    return 0;
}
