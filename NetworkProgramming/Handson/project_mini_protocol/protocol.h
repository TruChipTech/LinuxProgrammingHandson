/*
 * protocol.h — Custom mini-protocol definitions
 *
 * Used by both mini_server.c and mini_client.c
 */

#ifndef MINI_PROTOCOL_H
#define MINI_PROTOCOL_H

#include <stdint.h>

#define PROTO_MAGIC   0x4D494E49  /* "MINI" */
#define PROTO_VERSION 1
#define PROTO_PORT    9999

/* Message types */
#define MSG_HELLO     0x01
#define MSG_DATA      0x02
#define MSG_STATUS    0x03
#define MSG_BYE       0x04
#define MSG_ACK       0x80
#define MSG_ERROR     0xFF

/* Protocol header (12 bytes) */
struct proto_header {
    uint32_t magic;       /* PROTO_MAGIC */
    uint8_t  version;     /* PROTO_VERSION */
    uint8_t  msg_type;    /* MSG_* */
    uint16_t seq;         /* Sequence number */
    uint32_t payload_len; /* Payload length following header */
} __attribute__((packed));

/* HELLO payload */
struct proto_hello {
    char client_name[32];
} __attribute__((packed));

/* STATUS response payload */
struct proto_status {
    uint32_t uptime_sec;
    uint32_t connections;
    uint32_t messages;
} __attribute__((packed));

/* Maximum payload size */
#define MAX_PAYLOAD 4096

/* Complete message buffer */
struct proto_message {
    struct proto_header header;
    char payload[MAX_PAYLOAD];
};

/* Helper: fill header with network byte order */
static inline void proto_fill_header(struct proto_header *hdr,
                                     uint8_t type, uint16_t seq,
                                     uint32_t payload_len)
{
    hdr->magic = htonl(PROTO_MAGIC);
    hdr->version = PROTO_VERSION;
    hdr->msg_type = type;
    hdr->seq = htons(seq);
    hdr->payload_len = htonl(payload_len);
}

/* Helper: validate header */
static inline int proto_validate_header(const struct proto_header *hdr)
{
    if (ntohl(hdr->magic) != PROTO_MAGIC) return -1;
    if (hdr->version != PROTO_VERSION) return -2;
    if (ntohl(hdr->payload_len) > MAX_PAYLOAD) return -3;
    return 0;
}

/* Helper: message type to string */
static inline const char *proto_type_str(uint8_t type)
{
    switch (type) {
    case MSG_HELLO:  return "HELLO";
    case MSG_DATA:   return "DATA";
    case MSG_STATUS: return "STATUS";
    case MSG_BYE:    return "BYE";
    case MSG_ACK:    return "ACK";
    case MSG_ERROR:  return "ERROR";
    default:         return "UNKNOWN";
    }
}

#endif /* MINI_PROTOCOL_H */
