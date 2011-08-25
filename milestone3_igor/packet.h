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
    uint8_t seqno;    // datagram sequence number
    uint8_t segid;    // segment sequence number
    uint8_t ackseqno; // piggybacked ack sequence number
    uint8_t acksegid; // piggybacked ack segment number
    char msg[MAX_MESSAGE_SIZE];
} __attribute__((packed)) PACKET;

#define PACKET_HEADER_SIZE 4*sizeof(uint8_t)
//-----------------------------------------------------------------------------

#endif /* PACKET_H_ */
