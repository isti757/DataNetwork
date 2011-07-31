/*
 * transport_layer.c
 *
 *  Created on: Jul 30, 2011
 *      Author: isti
 */

#include "transport_layer.h"

//-----------------------------------------------------------------------------
// read incoming message from network to transport layer
void read_transport(CnetAddr destaddr, PACKET pkt) {
	int link = get_next_link_for_dest(destaddr);

	//int mtu = get_mtu_for_link(link);
	link = 0;
	// fragment packet

	// put into sliding window
	//read to network
	DATAGRAM dtg;
	dtg.dest = destaddr;
	dtg.packet = pkt;
	dtg.length = PACKET_SIZE(pkt);

	read_network(dtg);
}
//-----------------------------------------------------------------------------
// write outcoming message from application into transport layer
void write_transport(DATAGRAM dtg)
{
	PACKET p = dtg.packet;
	// if DATA pass up
    CNET_write_application((char*)&p.msg, (size_t*)&p.len);

	// if ACK shift sliding window
}
//-----------------------------------------------------------------------------
