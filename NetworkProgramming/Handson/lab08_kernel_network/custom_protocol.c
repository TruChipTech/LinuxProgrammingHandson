/*
 * custom_protocol.c — Kernel module: register a custom Ethernet protocol handler
 *
 * Registers a custom EtherType (0x88B5 — IEEE local experimental)
 * and processes any packets with that type.
 *
 * Build: make
 * Load:  sudo insmod custom_protocol.ko
 * Test:  sudo ./send_custom_packet (from user-space)
 * Check: dmesg | tail
 * Unload: sudo rmmod custom_protocol
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/if_ether.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lab");
MODULE_DESCRIPTION("Custom Ethernet protocol handler");

/* Use IEEE 802 Local Experimental EtherType */
#define CUSTOM_ETH_TYPE 0x88B5

/* Custom protocol header (after Ethernet header) */
struct custom_header {
    uint32_t magic;      /* 0xDEADBEEF */
    uint16_t version;    /* Protocol version */
    uint16_t msg_type;   /* Message type */
    uint32_t payload_len;/* Payload length */
} __attribute__((packed));

#define CUSTOM_MAGIC 0xDEADBEEF

static unsigned long rx_count;

/* Protocol handler — called by kernel when packet with our EtherType arrives */
static int custom_proto_rcv(struct sk_buff *skb, struct net_device *dev,
                            struct packet_type *pt, struct net_device *orig_dev)
{
    rx_count++;

    pr_info("custom_protocol: Packet #%lu received on %s (%d bytes)\n",
            rx_count, dev->name, skb->len);

    /* Parse our custom header */
    if (skb->len >= sizeof(struct custom_header)) {
        struct custom_header *hdr;

        if (!pskb_may_pull(skb, sizeof(struct custom_header))) {
            pr_warn("custom_protocol: cannot pull header\n");
            goto drop;
        }

        hdr = (struct custom_header *)skb->data;

        if (ntohl(hdr->magic) != CUSTOM_MAGIC) {
            pr_warn("custom_protocol: bad magic 0x%08x\n", ntohl(hdr->magic));
            goto drop;
        }

        pr_info("custom_protocol: version=%u type=%u payload_len=%u\n",
                ntohs(hdr->version), ntohs(hdr->msg_type),
                ntohl(hdr->payload_len));

        /* Print first few bytes of payload */
        if (skb->len > sizeof(struct custom_header)) {
            unsigned char *payload = skb->data + sizeof(struct custom_header);
            int plen = skb->len - sizeof(struct custom_header);
            if (plen > 32) plen = 32;
            pr_info("custom_protocol: payload (first %d bytes): %*ph\n",
                    plen, plen, payload);
        }
    }

drop:
    kfree_skb(skb);
    return 0;
}

static struct packet_type custom_ptype = {
    .type = __constant_htons(CUSTOM_ETH_TYPE),
    .func = custom_proto_rcv,
};

static int __init custom_proto_init(void)
{
    rx_count = 0;
    dev_add_pack(&custom_ptype);
    pr_info("custom_protocol: Registered EtherType 0x%04x\n", CUSTOM_ETH_TYPE);
    return 0;
}

static void __exit custom_proto_exit(void)
{
    dev_remove_pack(&custom_ptype);
    pr_info("custom_protocol: Unregistered. Total packets: %lu\n", rx_count);
}

module_init(custom_proto_init);
module_exit(custom_proto_exit);
