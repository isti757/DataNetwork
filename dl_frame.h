/*
 * frame.h
 *
 *  Created on: 12.08.2011
 *      Author: kirill
 */

#ifndef FRAME_H_
#define FRAME_H_
typedef struct {
	char data[DATAGRAM_HEADER_SIZE+MAX_MESSAGE_SIZE*2];
} __attribute__((packed)) DL_FRAME;

#endif /* FRAME_H_ */
