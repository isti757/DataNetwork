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

typedef enum    	{ DATA, ACK,NACK }   PACKETKIND;
typedef struct {
    CnetAddr		src;
    CnetAddr		dest;
    PACKETKIND	kind;      	/* only ever NL_DATA or NL_ACK */
    int			seqno;		/* 0, 1, 2, ... */
    int			hopsleft;	/* time to live */
    int			hopstaken;
    int			length;       	/* the length of the packet portion only */
    CnetTime		timesent;	/* in microseconds */
    PACKET packet;
} DATAGRAM;


/*typedef struct {

	CnetAddr src;
	CnetAddr dest;
	char kind;  only ever DATA or ACK or NACK
	char seqno;  0, 1, 2, ...

	char hopsleft;  time to live
	char hopstaken;

	CnetTime timesent;  in microseconds

	int checksum; // checksum of the whole datagram
	PACKET data;

} DATAGRAM;*/

#define DATAGRAM_HEADER_SIZE  (2*sizeof(CnetAddr)+sizeof(PACKETKIND)+ \
		4*sizeof(int)+sizeof(CnetTime))
#define DATAGRAM_SIZE(d)      (DATAGRAM_HEADER_SIZE + d.length)


#endif /* DATAGRAM_H_ */
