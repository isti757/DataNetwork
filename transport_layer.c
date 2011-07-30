/*
 * transport_layer.c
 *
 *  Created on: Jul 30, 2011
 *      Author: isti
 */

#include "transport_layer.h"

//-----------------------------------------------------------------------------
// read incoming message from network to transport layer
void read_transport(CnetAddr destaddr, PACKET pkt)
{
	int link = get_next_link_for_dest(destaddr);

	//int mtu = get_mtu_for_link(link);

	// fragment packet

	// put into sliding window

	DATAGRAM dtg;
	read_network(link, dtg);
}
//-----------------------------------------------------------------------------
// write outcoming message from application into transport layer
void write_transport(PACKET pkt, CnetAddr dest)
{
	// if DATA pass up
    CNET_write_application((char*)&pkt.data, (size_t*)&pkt.len);

	// if ACK shift sliding window
}
//-----------------------------------------------------------------------------
