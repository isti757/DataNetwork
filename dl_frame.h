/*
 * transport_layer.h
 *
 *  Created on: Aug 30, 2011
 *      Author: kirill
 */
#include "packet.h"

#ifndef FRAME_H_
#define FRAME_H_

//-----------------------------------------------------------------------------
//A frame on the datalink layer
typedef struct {
    uint16_t checksum;
    char data[PACKET_HEADER_SIZE+DATAGRAM_HEADER_SIZE+MAX_MESSAGE_SIZE];
} __attribute__((packed)) DL_FRAME;
#define DL_FRAME_HEADER_SIZE (sizeof(uint16_t))
//-----------------------------------------------------------------------------

#endif /* FRAME_H_ */
