/*
 * frame.c
 *
 *  Created on: Jun 28, 2011
 *      Author: isti
 */

#ifndef FRAME_H_
#define FRAME_H_

#include <cnet.h>

typedef enum {
	DL_DATA, DL_ACK
} FRAMEKIND;

typedef struct {
	char data[MAX_MESSAGE_SIZE];
} MSG;

typedef struct {
	FRAMEKIND kind;  // only ever DL_DATA or DL_ACK
	size_t len;      // the length of the msg field only
	int checksum;    // checksum of the whole frame
	int seq;         // only ever 0 or 1
	MSG msg;
} FRAME;

#define FRAME_HEADER_SIZE  (sizeof(FRAME) - sizeof(MSG))
#define FRAME_SIZE(f)      (FRAME_HEADER_SIZE + f.len)

#endif /* FRAME_H_ */
