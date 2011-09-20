/*
 * transport_layer.h
 *
 *  Created on: Aug 30, 2011
 *      Author: isti
 */
#ifndef PACKET_H_
#define PACKET_H_

#include <stdint.h>

#include <cnet.h>

//-----------------------------------------------------------------------------
// custom types
typedef uint8_t swin_segid_t;
typedef uint16_t swin_seqno_t;       // maximum seqno is 65535
//-----------------------------------------------------------------------------
// custom types width
#define SEQNO_WIDTH (8*sizeof(swin_frag_ind_t))
#define SEQNO_MASK  (swin_seqno_t)(-1)
//-----------------------------------------------------------------------------
typedef struct {
    swin_segid_t segid;     // segment sequence number
    swin_segid_t acksegid;  // piggybacked ack segment number
    swin_seqno_t seqno;     // packet sequence number
    swin_seqno_t ackseqno;  // piggybacked ack sequence number
    char msg[MAX_MESSAGE_SIZE];
} __attribute__((packed)) PACKET;

#define PACKET_HEADER_SIZE (2*sizeof(swin_segid_t)+2*sizeof(swin_seqno_t))
//-----------------------------------------------------------------------------

#endif /* PACKET_H_ */
