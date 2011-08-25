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
// initialize network layer
extern void init_network();
//-----------------------------------------------------------------------------
// detect a link for outcoming message
extern int get_next_link_for_dest(CnetAddr destaddr);
//-----------------------------------------------------------------------------
// detect fragmentation size for the specified link
extern int get_mtu_for_link(int link);
//-----------------------------------------------------------------------------
// write an incoming message from datalink to network layer
extern void read_network(int link, DATAGRAM dtg, int length);
//-----------------------------------------------------------------------------
// read an outgoing packet into network layer
extern void write_network(uint8_t k, CnetAddr ad, uint16_t len, PACKET p);
//-----------------------------------------------------------------------------
// clean the memory
extern void shutdown_network();
//-----------------------------------------------------------------------------
extern void print_network_layer();
//-----------------------------------------------------------------------------

#endif /* NETWORK_LAYER_H_ */
