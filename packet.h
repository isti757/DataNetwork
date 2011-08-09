/*
 * packet.h
 *
 *  Created on: Jun 28, 2011
 *      Author: isti
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <cnet.h>



typedef struct {
    uint8_t seqno; // datagram sequence number
    uint8_t segid; // segment sequence number
    char msg[MAX_MESSAGE_SIZE];
} __attribute__((packed)) PACKET;

#define PACKET_HEADER_SIZE 2*sizeof(uint8_t)

#endif /* PACKET_H_ */
