/*
 * datalink_layer.h
 *
 *  Created on: Jul 29, 2011
 *      Author: isti
 */

#ifndef DATALINK_LAYER_H_
#define DATALINK_LAYER_H_

#include "frame.h"

#include "network_layer.h"

//-----------------------------------------------------------------------------
typedef struct datalink_layer {

} datalink_layer;
//-----------------------------------------------------------------------------
// initialize datalink layer
void init_datalink();
//-----------------------------------------------------------------------------
// write an incoming frame into datalink layer
void read_datalink(int link, FRAME frame) {
	;
}
//-----------------------------------------------------------------------------
// write an outcoming packet into datalink layer
void write_datalink(int link, PACKET packet);
//-----------------------------------------------------------------------------
static EVENT_HANDLER(physical_ready) {
	FRAME frm;

	int link;
	size_t len = sizeof(FRAME);

	CHECK(CNET_read_physical(&link, (char *)&frm, &len));
}
//-----------------------------------------------------------------------------

#endif /* DATALINK_LAYER_H_ */
