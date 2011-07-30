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

//-----------------------------------------------------------------------------
//typedef struct datalink_layer {
//
//} datalink_layer;
//-----------------------------------------------------------------------------
// initialize datalink layer
extern void init_datalink();
//-----------------------------------------------------------------------------
// write an incoming frame into datalink layer
extern void read_datalink(int link, DATAGRAM dtg);
//-----------------------------------------------------------------------------

#endif /* DATALINK_LAYER_H_ */
