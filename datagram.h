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
#define MAXFRAMESIZE MAX_MESSAGE_SIZE + 4
typedef enum {
    ACK = 1, NACK = 2, DATA = 4, LASTSGM = 8,   // transport layer
    DISCOVER = 16, ROUTING = 32, TRANSPORT = 64 // network layer
} PACKETKIND;
//-----------------------------------------------------------------------------
typedef struct {
    uint8_t src;       // source address
    uint8_t dest;      // destination address
    uint8_t kind;      // packet kind

    uint16_t length;   // length of payload

    int32_t checksum;  // checksum of entire datagram

    CnetTime timesent; // in microseconds

    char payload[MAXFRAMESIZE];

} __attribute__((packed)) DATAGRAM;

#define DATAGRAM_HEADER_SIZE  (3*sizeof(uint8_t)+sizeof(uint16_t)+sizeof(uint32_t)+sizeof(CnetTime))
#define DATAGRAM_SIZE(d)      (DATAGRAM_HEADER_SIZE + d.length)

#endif /* DATAGRAM_H_ */
