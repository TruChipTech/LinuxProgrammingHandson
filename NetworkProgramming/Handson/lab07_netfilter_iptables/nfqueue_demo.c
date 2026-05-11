/*
 * nfqueue_demo.c — User-space packet processing using NFQUEUE
 *
 * Intercepts packets sent to NFQUEUE and decides to ACCEPT or DROP.
 *
 * Build: make
 * Setup: sudo iptables -A INPUT -p icmp -j NFQUEUE --queue-num 0
 * Run:   sudo ./nfqueue_demo
 * Test:  ping -c 3 127.0.0.1
 * Clean: sudo iptables -D INPUT -p icmp -j NFQUEUE --queue-num 0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>

static volatile int running = 1;
static unsigned long accepted = 0, dropped = 0;

static void sigint_handler(int sig) { (void)sig; running = 0; }

static int queue_callback(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
                          struct nfq_data *nfa, void *data)
{
    (void)nfmsg;
    (void)data;

    struct nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr(nfa);
    if (!ph) return -1;

    uint32_t id = ntohl(ph->packet_id);

    /* Get packet payload */
    unsigned char *payload;
    int payload_len = nfq_get_payload(nfa, &payload);

    if (payload_len >= (int)sizeof(struct iphdr)) {
        struct iphdr *ip = (struct iphdr *)payload;
        char src[INET_ADDRSTRLEN], dst[INET_ADDRSTRLEN];
        struct in_addr s, d;
        s.s_addr = ip->saddr;
        d.s_addr = ip->daddr;
        inet_ntop(AF_INET, &s, src, sizeof(src));
        inet_ntop(AF_INET, &d, dst, sizeof(dst));

        printf("Packet #%u: %s → %s  proto=%d  len=%d  ",
               id, src, dst, ip->protocol, payload_len);

        /* Decision: accept all packets (modify this for filtering) */
        printf("→ ACCEPT\n");
        accepted++;
        return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);

        /* To drop a packet: */
        /* dropped++; */
        /* return nfq_set_verdict(qh, id, NF_DROP, 0, NULL); */
    }

    return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
}

int main(void)
{
    signal(SIGINT, sigint_handler);

    printf("NFQUEUE Packet Processor (queue 0)\n");
    printf("Setup:  sudo iptables -A INPUT -p icmp -j NFQUEUE --queue-num 0\n");
    printf("Press Ctrl+C to stop.\n\n");

    struct nfq_handle *h = nfq_open();
    if (!h) {
        fprintf(stderr, "nfq_open() failed\n");
        return 1;
    }

    if (nfq_unbind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "nfq_unbind_pf() failed\n");
    }
    if (nfq_bind_pf(h, AF_INET) < 0) {
        fprintf(stderr, "nfq_bind_pf() failed\n");
        nfq_close(h);
        return 1;
    }

    struct nfq_q_handle *qh = nfq_create_queue(h, 0, &queue_callback, NULL);
    if (!qh) {
        fprintf(stderr, "nfq_create_queue() failed\n");
        nfq_close(h);
        return 1;
    }

    if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) {
        fprintf(stderr, "nfq_set_mode() failed\n");
        nfq_destroy_queue(qh);
        nfq_close(h);
        return 1;
    }

    int fd = nfq_fd(h);
    char buf[4096];

    while (running) {
        int rv = recv(fd, buf, sizeof(buf), 0);
        if (rv < 0) {
            if (!running) break;
            perror("recv");
            break;
        }
        nfq_handle_packet(h, buf, rv);
    }

    printf("\n=== Statistics ===\n");
    printf("Accepted: %lu  Dropped: %lu\n", accepted, dropped);

    nfq_destroy_queue(qh);
    nfq_close(h);
    return 0;
}
