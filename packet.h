/*
 * packet.h
 *
 *  Created on: Jun 28, 2011
 *      Author: isti
 */

#ifndef PACKET_H_
#define PACKET_H_

#include "debug.h"

#include <cnet.h>

typedef struct
{
	int len;
	char data[MAX_MESSAGE_SIZE];
} PACKET;

#define PACKET_HEADER_SIZE sizeof(int)
#define PACKET_SIZE(p)      (PACKET_HEADER_SIZE + p->len)

#endif /* PACKET_H_ */
