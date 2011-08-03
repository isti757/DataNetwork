/*
 * transport_layer.c
 *
 *  Created on: Jul 30, 2011
 *      Author: isti
 */

#include "transport_layer.h"
void init_transport()
{
	//set cnet handler
	CHECK(CNET_set_handler( EV_APPLICATIONREADY, write_transport, 0));

}


//-----------------------------------------------------------------------------
// read incoming message from network to transport layer
void write_transport(CnetEvent ev, CnetTimerID timer, CnetData data)
{
	// read the message from application layer
	CnetAddr destaddr;
	PACKET pkt;
	pkt.len = sizeof(pkt.msg);
	CHECK(CNET_read_application(&destaddr, &pkt.msg, &pkt.len));
	CNET_disable_application(destaddr);

	int link = get_next_link_for_dest(destaddr);

	//int mtu = get_mtu_for_link(link);
	link = 0;
	// fragment packet

	// put into sliding window
	//read to network

	write_network(destaddr,pkt);
}
//-----------------------------------------------------------------------------
// write incoming message from application into transport layer
void read_transport(DATAGRAM dtg)
{
	PACKET p;
	memcpy(&p,&dtg.payload,dtg.length);
	// if DATA pass up
	size_t len = dtg.length-4;
    CNET_write_application((char*)&p.msg, &len);

	// if ACK shift sliding window
}
//-----------------------------------------------------------------------------
