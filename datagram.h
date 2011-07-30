/*
 * datagram.h
 *
 *  Created on: 30.07.2011
 *      Author: kirill
 */

#ifndef DATAGRAM_H_
#define DATAGRAM_H_

#include <cnet.h>
#include "packet.h"

typedef struct {

	CnetAddr src;
	CnetAddr dest;
	char kind; /* only ever DATA or ACK or NACK */
	int seqno; /* 0, 1, 2, ... */

	char hopsleft; /* time to live */
	char hopstaken;

	CnetTime timesent; /* in microseconds */

	int checksum; // checksum of the whole datagram
	ushort length; /* the length of the data portion only */
	PACKET data;

} DATAGRAM;

#endif /* DATAGRAM_H_ */
