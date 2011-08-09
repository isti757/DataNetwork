/*
 * transport_layer.c
 *
 *  Created on: Jul 30, 2011
 *      Author: isti
 */

#include "transport_layer.h"
#include "routing.h"
#include "datagram.h"

CnetTimerID discovery_pending_timer;
//-----------------------------------------------------------------------------
// read incoming message from network to transport layer
void write_transport(CnetEvent ev, CnetTimerID timer, CnetData data)
{
	// read the message from application layer
	CnetAddr destaddr;
	PACKET pkt;
	//pkt.len = sizeof(pkt.msg);
	size_t packet_len = sizeof(PACKET);
	CHECK(CNET_read_application(&destaddr, &pkt.msg, &packet_len));
	printf("Sending packet to %d\n",destaddr);
	write_network(TRANSPORT,destaddr,PACKET_HEADER_SIZE+packet_len,(char*)&pkt);
}
//-----------------------------------------------------------------------------
// write incoming message from application into transport layer
void read_transport(PACKETKIND kind,uint16_t length,CnetAddr source,char * packet)
{
	PACKET p;
	size_t len = length;
	memcpy(&p,packet,len);
	len-=PACKET_HEADER_SIZE;
    CNET_write_application((char*)p.msg, &len);

}
//-----------------------------------------------------------------------------
//enables application after finishing neighbor discovery process
void discovery_pending(CnetEvent ev, CnetTimerID timer, CnetData data)
{
	printf("checking for discovery to finish...\n");
	if (check_neighbors_discovered() == 0) {
		printf("Waiting more..\n");
		discovery_pending_timer = CNET_start_timer(EV_DISCOVERY_PENDING_TIMER,
				DISCOVERY_PENDING_TIME, 0);
	} else {
		//if (nodeinfo.address==182)
		CNET_enable_application(ALLNODES);
		discovery_pending_timer = NULLTIMER;
	}
}
//-----------------------------------------------------------------------------
void init_transport()
{
	//set cnet handler
	CHECK(CNET_set_handler( EV_APPLICATIONREADY, write_transport, 0));
	CHECK(CNET_set_handler(EV_DISCOVERY_PENDING_TIMER, discovery_pending, 0));
	discovery_pending_timer = CNET_start_timer(EV_DISCOVERY_PENDING_TIMER,
				DISCOVERY_PENDING_TIME, 0);
}
