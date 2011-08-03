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
	CHECK(CNET_set_handler( EV_APPLICATIONREADY, read_transport, 0));

}


//-----------------------------------------------------------------------------
// read incoming message from network to transport layer
void read_transport(CnetEvent ev, CnetTimerID timer, CnetData data)
{
	// read the message from application layer
	CnetAddr destaddr;
	PACKET pkt;
	pkt.len = sizeof(pkt.msg);
	CHECK(CNET_read_application(&destaddr, pkt.msg, (size_t*)&pkt.len));
	CNET_disable_application(destaddr);


	int link = get_next_link_for_dest(destaddr);

	//int mtu = get_mtu_for_link(link);
	link = 0;
	// fragment packet

	// put into sliding window
	//read to network
	DATAGRAM dtg;
	dtg.dest = destaddr;
	memcpy(&dtg.payload,&pkt,pkt.len+4);
	dtg.length = PACKET_SIZE(pkt);
	dtg.kind = TRANSPORT;

	read_network(dtg);
}
//-----------------------------------------------------------------------------
// write outcoming message from application into transport layer
void write_transport(DATAGRAM dtg)
{
	PACKET p;
	memcpy(&p,&dtg.payload,dtg.length);
	// if DATA pass up
    CNET_write_application((char*)&p.msg, (size_t*)&p.len);

	// if ACK shift sliding window
}
//-----------------------------------------------------------------------------
