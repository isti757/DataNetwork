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
typedef struct network_layer {

} network_layer;
//-----------------------------------------------------------------------------
// initialize network layer
void init_network();
//-----------------------------------------------------------------------------
// detect a link for outcoming message
int get_next_link_for_dest(CnetAddr destaddr);
//-----------------------------------------------------------------------------
// detect fragmentation size for the specified link
int get_mtu_for_link(int link);
//-----------------------------------------------------------------------------
// read an incoming packet into network layer
void read_network(int link, DATAGRAM dtg)
{
	FRAME frm;
	read_datalink(link, frm);
}
//-----------------------------------------------------------------------------
// write an outgoing message from transport to network layer
void write_network(int link, PACKET packet);
//-----------------------------------------------------------------------------

#endif /* NETWORK_LAYER_H_ */
