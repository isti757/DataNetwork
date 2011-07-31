/*
 * network_layer.h
 *
 *  Created on: Jun 28, 2011
 *      Author: isti
 */

#ifndef NETWORK_LAYER_H_
#define NETWORK_LAYER_H_

#include "datagram.h"
#include "datalink_layer.h"
#include "transport_layer.h"

//-----------------------------------------------------------------------------
//typedef struct network_layer {
//
//} network_layer;
//-----------------------------------------------------------------------------
// initialize network layer
extern void init_network();
//-----------------------------------------------------------------------------
// detect a link for outcoming message
extern int get_next_link_for_dest(CnetAddr destaddr);
//-----------------------------------------------------------------------------
// detect fragmentation size for the specified link
extern int get_mtu_for_link(int link);
//-----------------------------------------------------------------------------
// read an incoming packet into network layer
extern void read_network(DATAGRAM dtg);
//-----------------------------------------------------------------------------
// write an incoming message from datalink to network layer
extern void write_network(int link, DATAGRAM dtg, int length);
//-----------------------------------------------------------------------------
extern void show_table(CnetEvent ev, CnetTimerID timer, CnetData data);
#endif /* NETWORK_LAYER_H_ */
