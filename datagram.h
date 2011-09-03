/*
 * datagram.h
 *
 *  Created on: 30.07.2011
 *      Author: kirill
 */

#ifndef DATAGRAM_H_
#define DATAGRAM_H_

#include <cnet.h>

#include "packet.h"

//-----------------------------------------------------------------------------
// types of signal exchange data
typedef enum {
        CONGESTION = 1,
        DISCOVERY_FINISHED = 2
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
static int is_kind(uint8_t kind, uint8_t knd) {
    return ((kind & knd) > 0);
}
//-----------------------------------------------------------------------------
// definition of datagram
#define MAXFRAMESIZE MAX_MESSAGE_SIZE + 4
typedef struct {
    uint8_t src;       // source address
    uint8_t dest;      // destination address
    uint8_t kind;      // packet kind
    uint16_t length;   // length of payload
    uint32_t checksum; // checksum of entire datagram
    uint32_t payload_checksum;//checksum of the payload only
    char payload[MAXFRAMESIZE];
} __attribute__((packed)) DATAGRAM;

#define DATAGRAM_HEADER_SIZE  (3*sizeof(uint8_t)+sizeof(uint16_t)+2*sizeof(uint32_t))
#define DATAGRAM_SIZE(d)      (DATAGRAM_HEADER_SIZE + d.length)
//-----------------------------------------------------------------------------

#endif /* DATAGRAM_H_ */
