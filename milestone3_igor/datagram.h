/*
 * datagram.h
 *
 *  Created on: 30.07.2011
 *      Author: kirill
 */

#ifndef DATAGRAM_H_
#define DATAGRAM_H_

#include <stdint.h>

#include <cnet.h>
#include "packet.h"

//-----------------------------------------------------------------------------
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
typedef struct {
    uint8_t src;       // source address
    uint8_t dest;      // destination address
    uint8_t kind;      // packet kind

    uint16_t length;   // length of payload

//    CnetTime timesent; // in microseconds

    uint16_t checksum;  // checksum of entire datagram

    PACKET payload;

} __attribute__((packed)) DATAGRAM;

#define DATAGRAM_HEADER_SIZE  (3*sizeof(uint8_t)+2*sizeof(uint16_t))//+sizeof(int32_t))+sizeof(CnetTime))
#define DATAGRAM_SIZE(d)      (DATAGRAM_HEADER_SIZE + d.length)
//-----------------------------------------------------------------------------

#endif /* DATAGRAM_H_ */
