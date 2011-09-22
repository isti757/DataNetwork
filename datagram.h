/*
 * transport_layer.h
 *
 *  Created on: Aug 30, 2011
 *      Author: isti
 */
#ifndef DATAGRAM_H_
#define DATAGRAM_H_

#include <cnet.h>

#include "packet.h"

//-----------------------------------------------------------------------------
// custom types
typedef uint8_t  msg_type_t;
typedef uint16_t msg_len_t;          // maximum message length is 10240 bytes
typedef uint8_t  swin_addr_t;
//-----------------------------------------------------------------------------
// custom types width
#define ADDR_MASK   (swin_addr_t)(-1)
//-----------------------------------------------------------------------------
// types of signal exchange data
typedef enum {
        CONGESTION = 1,
        DISCOVERY_FINISHED = 2,
        MTU_DISCOVERED = 3
} SIGNALKIND;
typedef	CnetData SIGNALDATA;
//-----------------------------------------------------------------------------
// types of message exchange data
const static uint8_t
    __ACK__ = 1,
    __NACK__ = 2,
    __DATA__ = 4,
    __LASTSGM__ = 8;   // transport layer

const static uint8_t
    __DISCOVER__ = 16,
    __ROUTING__ = 32,
    __TRANSPORT__ = 64; // network layer
//-----------------------------------------------------------------------------
static int is_kind(msg_type_t kind1, msg_type_t kind2) {
    return ((kind1 & kind2) > 0);
}
//-----------------------------------------------------------------------------
// definition of datagram
#define MAXFRAMESIZE MAX_MESSAGE_SIZE + PACKET_HEADER_SIZE
typedef struct {
    swin_addr_t src;    // source address
    swin_addr_t dest;   // destination address
    msg_type_t kind;    // packet kind
    msg_len_t length;   // length of payload
    uint16_t req_id;
    char payload[MAXFRAMESIZE];
} __attribute__((packed)) DATAGRAM;

#define DATAGRAM_HEADER_SIZE  (2*sizeof(swin_addr_t)+sizeof(msg_type_t)+sizeof(msg_len_t)+sizeof(uint16_t))
#define DATAGRAM_SIZE(d)      (DATAGRAM_HEADER_SIZE + d.length)
//-----------------------------------------------------------------------------

#endif /* DATAGRAM_H_ */
