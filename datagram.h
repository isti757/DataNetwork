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

typedef enum    	{ DATA, ACK, NACK }   DATAGRAMKIND;

typedef struct {
	CnetAddr		src;
	CnetAddr		dest;
	DATAGRAMKIND	kind;   /* only ever DATA or ACK or NACK */
	int			seqno;		/* 0, 1, 2, ... */
	int			hopsleft;	/* time to live */
	int			hopstaken;
	CnetTime	timesent;	/* in microseconds */
	int         checksum;  	// checksum of the whole datagram
	int			length;     /* the length of the data portion only */
	PACKET      data;
} DATAGRAM;


#endif /* DATAGRAM_H_ */
