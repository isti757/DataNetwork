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

#define EV_DATALINK_FREE EV_TIMER5

//-----------------------------------------------------------------------------
// initialize datalink layer
extern void init_datalink();
//-----------------------------------------------------------------------------
// read an incoming frame into datalink layer
extern void read_datalink(CnetEvent event, CnetTimerID timer, CnetData data);
//-----------------------------------------------------------------------------
// write an outcoming frame into datalink layer
extern void write_datalink(int link, DATAGRAM dtg);
//-----------------------------------------------------------------------------
// clean the memory
extern void shutdown_datalink();
//-----------------------------------------------------------------------------
extern void print_datalink_layer();
//-----------------------------------------------------------------------------

#endif /* DATALINK_LAYER_H_ */
