/*
 * datalink_layer.h
 *
 *  Created on: Jul 29, 2011
 *      Author: isti
 */

#ifndef DATALINK_LAYER_H_
#define DATALINK_LAYER_H_

#include "datagram.h"
#include "network_layer.h"

typedef struct {
	int link;
	uint32_t len;
	char * packet;
} datalink_container;
//-----------------------------------------------------------------------------
// initialize datalink layer
extern void init_datalink();
//-----------------------------------------------------------------------------
// read an incoming frame into datalink layer
extern void read_datalink(CnetEvent event, CnetTimerID timer, CnetData data);
//-----------------------------------------------------------------------------
// write an outcoming frame into datalink layer
extern void write_datalink(int, char *, uint32_t);
#endif /* DATALINK_LAYER_H_ */
