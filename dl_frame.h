/*
 * transport_layer.h
 *
 *  Created on: Aug 30, 2011
 *      Author: kirill
 */
#ifndef FRAME_H_
#define FRAME_H_

//-----------------------------------------------------------------------------
//A frame on the datalink layer
typedef struct {
    uint32_t checksum;
    char data[DATAGRAM_HEADER_SIZE+MAX_MESSAGE_SIZE*2]; //TODO fix it
} __attribute__((packed)) DL_FRAME;
#define DL_FRAME_HEADER_SIZE (sizeof(uint32_t))
//-----------------------------------------------------------------------------

#endif /* FRAME_H_ */
