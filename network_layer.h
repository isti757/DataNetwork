/*
 * transport_layer.h
 *
 *  Created on: Aug 30, 2011
 *      Author: kirill
 */
#ifndef NETWORK_LAYER_H_
#define NETWORK_LAYER_H_

#include <cnetsupport.h>

#include "datagram.h"
#include "discovery.h"
#include "routing.h"

#include "datalink_layer.h"
#include "transport_layer.h"

//-----------------------------------------------------------------------------
// initialize network layer
extern void init_network();
//-----------------------------------------------------------------------------
// write an outcoming packet into network layer
extern void write_network(uint8_t kind, CnetAddr dest,uint16_t length, char* data);
//-----------------------------------------------------------------------------
// read an incoming message from datalink to network layer
extern void read_network(int link, size_t length, char * datagram);
//-----------------------------------------------------------------------------
// send the datagram to the specified address
extern void send_packet(CnetAddr addr, DATAGRAM datagram);
//-----------------------------------------------------------------------------
// send the datagram to the specified link
extern void send_packet_to_link(int link, DATAGRAM datagram);
//-----------------------------------------------------------------------------
// broadcast packet to links
extern void broadcast_packet(DATAGRAM dtg, int exclude_link);
//-----------------------------------------------------------------------------
// allocated a datagram
extern DATAGRAM alloc_datagram(uint8_t prot, int src, int dest, char *data, uint16_t len);
//-----------------------------------------------------------------------------
// clean the memory
extern void shutdown_network();
//-----------------------------------------------------------------------------

#endif /* NETWORK_LAYER_H_ */
