/*
 * network_layer.h
 *
 *  Created on: Jun 28, 2011
 *      Author: isti
 */

#ifndef NETWORK_LAYER_H_
#define NETWORK_LAYER_H_

#include "datagram.h"
#include <cnetsupport.h>
#include "datalink_layer.h"
#include "transport_layer.h"
#include "discovery.h"

//-----------------------------------------------------------------------------
// initialize network layer
extern void init_network();
//-----------------------------------------------------------------------------
// read an incoming packet into network layer
extern void write_network(PACKETKIND, CnetAddr,uint16_t, char*);
//-----------------------------------------------------------------------------
// write an incoming message from datalink to network layer
extern void read_network(int link, DATAGRAM dtg);
//-----------------------------------------------------------------------------
// send the datagram to the specified address
extern void send_packet(int , DATAGRAM);
//-----------------------------------------------------------------------------
// send the datagram to the specified link
extern void send_packet_to_link(int, DATAGRAM);
//-----------------------------------------------------------------------------
// broadcast packet to links
extern void broadcast_packet(DATAGRAM,int);
//-----------------------------------------------------------------------------
//allocated a datagram
extern DATAGRAM* alloc_datagram(int, int, int, char *, uint16_t);
#endif /* NETWORK_LAYER_H_ */
