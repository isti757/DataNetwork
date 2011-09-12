/*
 * packet.h
 *
 *  Created on: Jun 28, 2011
 *      Author: isti
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <stdint.h>

#include <cnet.h>

//-----------------------------------------------------------------------------
typedef struct {
    uint8_t segid;     // segment sequence number
    uint8_t acksegid;  // piggybacked ack segment number
    uint16_t seqno;    // packet sequence number
    uint16_t ackseqno; // piggybacked ack sequence number
    char msg[MAX_MESSAGE_SIZE];
} __attribute__((packed)) PACKET;

#define PACKET_HEADER_SIZE (2*sizeof(uint8_t)+2*sizeof(uint16_t))
//-----------------------------------------------------------------------------

#endif /* PACKET_H_ */
