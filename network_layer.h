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
// read an incoming packet into network layer
extern void write_network(CnetAddr, PACKET);
//-----------------------------------------------------------------------------
// write an incoming message from datalink to network layer
extern void read_network(int link, DATAGRAM dtg, int length);
//-----------------------------------------------------------------------------
// send the datagram to the specified address
extern void send_packet(int , DATAGRAM);
//-----------------------------------------------------------------------------
// send the datagram to the specified link
extern void send_packet_to_link(int, DATAGRAM);
//-----------------------------------------------------------------------------
// broadcast packet to links
extern void broadcast_packet(DATAGRAM,int);
//allocated a datagram
extern DATAGRAM* alloc_datagram(int, int, int, char *, int);

#endif /* NETWORK_LAYER_H_ */
