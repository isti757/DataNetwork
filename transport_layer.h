/*
 * transport_layer.h
 *
 *  Created on: Jul 29, 2011
 *      Author: isti
 */

#ifndef TRANSPORT_LAYER_H_
#define TRANSPORT_LAYER_H_

#include "frame.h"

#include "network_layer.h"
#include "application_layer.h"

//-----------------------------------------------------------------------------
typedef struct transport_layer {

};
//-----------------------------------------------------------------------------
// initialize transport layer
void init_transport();
//-----------------------------------------------------------------------------
// read incoming message from network to transport layer
void read_transport(CnetAddr destaddr, PACKET pkt)
{
	int link = get_next_link_for_dest(destaddr);

	int mtu = get_mtu_for_link(link);

	// fragment packet

	// put into sliding window

	DATAGRAM dtg;
	read_network(link, dtg);
}
//-----------------------------------------------------------------------------
// write outcoming message from application into transport layer
void write_transport(PACKET pkt);
//-----------------------------------------------------------------------------


#endif /* TRANSPORT_LAYER_H_ */
