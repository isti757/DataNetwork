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
#define MAXFRAMESIZE MAX_MESSAGE_SIZE + 4
typedef enum    	{ DATA, ACK, NACK, DISCOVER, TRANSPORT,ROUTING }   PACKETKIND;
typedef struct {
    CnetAddr		src;
    CnetAddr		dest;
    PACKETKIND	kind;
    int			seqno;		/* 0, 1, 2, ... */
    int			hopsleft;	/* time to live */
    int			hopstaken;
    size_t			length;       	/* the length of the packet portion only */
    CnetTime		timesent;	/* in microseconds */
    char  payload[MAXFRAMESIZE];
} DATAGRAM;


#define DATAGRAM_HEADER_SIZE  (2*sizeof(CnetAddr)+sizeof(PACKETKIND)+ \
		4*sizeof(int)+sizeof(CnetTime))
#define DATAGRAM_SIZE(d)      (DATAGRAM_HEADER_SIZE + d.length)


#endif /* DATAGRAM_H_ */
