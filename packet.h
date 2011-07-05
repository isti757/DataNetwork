/*
 * packet.h
 *
 *  Created on: Jun 28, 2011
 *      Author: isti
 */

#ifndef PACKET_H_
#define PACKET_H_

#include <cnet.h>

typedef struct
{
	int len;
	char data[MAX_MESSAGE_SIZE];
} PACKET;


#endif /* PACKET_H_ */
