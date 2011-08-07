/*
 * transport_layer.c
 *
 *  Created on: Jul 30, 2011
 *      Author: isti
 */

#include "transport_layer.h"
#include "routing.h"
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
	printf("Sending packet to %d\n",destaddr);
	write_network(TRANSPORT,destaddr,PACKET_SIZE(pkt),(char*)&pkt);
}
//-----------------------------------------------------------------------------
// write incoming message from application into transport layer
void read_transport(uint16_t length, char * packet)
{
	PACKET p;
	memcpy(&p,packet,length);
	size_t len = length-PACKET_HEADER_SIZE;
    CNET_write_application((char*)p.msg, &len);

}
//-----------------------------------------------------------------------------
