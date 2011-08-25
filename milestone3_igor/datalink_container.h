/*
 * datalink_container.h
 *
 *  Created on: 09.08.2011
 *      Author: kirill
 */

#ifndef DATALINK_CONTAINER_H_
#define DATALINK_CONTAINER_H_

#include <stdint.h>

typedef struct {
	int link;
	uint32_t len;
	char datagram[PACKET_HEADER_SIZE+DATAGRAM_HEADER_SIZE+MAX_MESSAGE_SIZE];
} __attribute__((packed)) DTG_CONTAINER;

#define DTG_CONTAINER_HEADER_SIZE  (sizeof(int)+sizeof(uint32_t))
#define DTG_CONTAINER_SIZE(d)      (DTG_CONTAINER_HEADER_SIZE + d.len)

#endif /* DATALINK_CONTAINER_H_ */
