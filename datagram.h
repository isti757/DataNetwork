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
typedef enum {
    ACK = 1, NACK = 2, DATA = 4, LASTSGM = 8,   // transport layer
    DISCOVER = 16, ROUTING = 32, TRANSPORT = 64 // network layer
} PACKETKIND;
typedef struct {
    uint8_t		src;
    uint8_t		dest;
    uint16_t	kind;
    uint16_t	length;       	/* the length of the packet portion only */
    int32_t		checksum;
    CnetTime		timesent;	/* in microseconds */
    char  payload[MAXFRAMESIZE];
} DATAGRAM;


#define DATAGRAM_HEADER_SIZE  (2*sizeof(CnetAddr)+sizeof(PACKETKIND)+ \
		2*sizeof(int)+sizeof(CnetTime))
#define DATAGRAM_SIZE(d)      (DATAGRAM_HEADER_SIZE + d.length)


#endif /* DATAGRAM_H_ */
