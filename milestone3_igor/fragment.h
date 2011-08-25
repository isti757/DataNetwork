/*
 * fragment.h
 *
 *  Created on: Aug 5, 2011
 *      Author: isti
 */

#ifndef FRAGMENT_H_
#define FRAGMENT_H_

#include <stdint.h>

#include <cnet.h>

#include "packet.h"

//-----------------------------------------------------------------------------
typedef struct {
    int8_t dest; // destination
    int8_t kind; // kind of packet
    int16_t len;  // size of pkt message, not packet itself
    PACKET pkt;
} __attribute__((packed)) FRAGMENT;

#define FRAGMENT_HEADER_SIZE (2*sizeof(uint8_t)+sizeof(uint16_t))
#define FRAGMENT_SIZE(f) (FRAGMENT_HEADER_SIZE + PACKET_SIZE(f.pkt))
//-----------------------------------------------------------------------------

#endif /* FRAGMENT_H_ */
