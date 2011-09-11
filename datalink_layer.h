/*
 * datalink_layer.h
 *
 *  Created on: Jul 29, 2011
 *      Author: isti
 */

#ifndef DATALINK_LAYER_H_
#define DATALINK_LAYER_H_

#include "datagram.h"
#include "dl_frame.h"
#include "datalink_container.h"
#include "network_layer.h"

//-----------------------------------------------------------------------------
#define EV_DATALINK_FLUSH EV_TIMER6
#define EV_DATALINK_FREE EV_TIMER5
//-----------------------------------------------------------------------------
// initialize datalink layer
extern void init_datalink();
//-----------------------------------------------------------------------------
// read an incoming frame into datalink layer
extern void read_datalink(CnetEvent event, CnetTimerID timer, CnetData data);
//-----------------------------------------------------------------------------
// write an outcoming frame into datalink layer
extern void write_datalink(int link, char * data, uint32_t checksum, uint32_t length);
//-----------------------------------------------------------------------------
// clean the memory
extern void shutdown_datalink();
//-----------------------------------------------------------------------------
#endif /* DATALINK_LAYER_H_ */
