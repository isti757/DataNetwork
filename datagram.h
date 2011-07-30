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
	char seqno; /* 0, 1, 2, ... */

	char hopsleft; /* time to live */
	char hopstaken;

	CnetTime timesent; /* in microseconds */

	int checksum; // checksum of the whole datagram
	PACKET data;

} DATAGRAM;

#define DATAGRAM_HEADER_SIZE  (sizeof(DATAGRAM) - sizeof(PACKET))
#define DATAGRAM_SIZE(d)      (DATAGRAM_HEADER_SIZE + d.data.len)


#endif /* DATAGRAM_H_ */
