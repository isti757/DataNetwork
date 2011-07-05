/*
 * frame.c
 *
 *  Created on: Jun 28, 2011
 *      Author: isti
 */

#ifndef FRAME_H_
#define FRAME_H_

#include <cnet.h>

#include "packet.h"

typedef enum {
	DL_DATA, DL_ACK, DL_NACK
} FRAMEKIND;

typedef struct FRAME {

	// header
	FRAMEKIND kind;  // only ever DL_DATA or DL_ACK

	int len;      // the length of the msg field only
	int checksum;    // checksum of the whole frame

	// sliding window sequence number
	int seq;

	// segmentation
	int seg_seq;

	// message part
	PACKET msg;

} FRAME;

#define FRAME_HEADER_SIZE  (sizeof(FRAME) - sizeof(PACKET))
#define FRAME_SIZE(f)      (FRAME_HEADER_SIZE + f.len)

PACKET* get_frame_data(FRAME *fr)
{
	return &(fr->msg);
}

size_t get_frama_data_len(const FRAME *fr)
{
	return fr->len;
}

#endif /* FRAME_H_ */
