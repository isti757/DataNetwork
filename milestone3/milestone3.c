/*
 * milestone1.c
 *
 *  Created on: Jul 18, 2011
 *      Author: isti
 */

#include <cnet.h>
#include <stdlib.h>
#include <string.h>

#include "datalink_layer.h"
#include "network_layer.h"
#include "transport_layer.h"
#include "application_layer.h"

//-----------------------------------------------------------------------------
static EVENT_HANDLER(application_ready)
{
	// read the message from application layer
	CnetAddr destaddr;
	PACKET pkt;
	pkt.len = sizeof(pkt.msg);
	CHECK(CNET_read_application(&destaddr, pkt.msg, (size_t*)&pkt.len));
	CNET_disable_application(destaddr);

	//read_transport goes here
	read_transport(destaddr, pkt);


}
//-----------------------------------------------------------------------------
static EVENT_HANDLER(physical_ready)
{
	int link;
	size_t len;
	DATAGRAM dtg;
	len = sizeof(DATAGRAM);
	//read a datagram from the datalink layer
	CHECK(CNET_read_physical(&link, (char *)&dtg, &len));

	// checksum check
	// maybe send NACK

	write_network(link, dtg,len);
}
//-----------------------------------------------------------------------------
// Clean whatever is left in the queue and free the memory
//
static EVENT_HANDLER(shutdown) {
	;
}
//-----------------------------------------------------------------------------
static EVENT_HANDLER(showstate) {
	;
}
//-----------------------------------------------------------------------------
EVENT_HANDLER(reboot_node) {

	CHECK(CNET_set_handler( EV_APPLICATIONREADY, application_ready, 0));
	CHECK(CNET_set_handler( EV_PHYSICALREADY, physical_ready, 0));
//	CHECK(CNET_set_handler( EV_TIMER1, timeouts, 0));
	CHECK(CNET_set_handler( EV_DEBUG0, showstate, 0));
	CHECK(CNET_set_handler( EV_SHUTDOWN, shutdown, 0));

	CHECK(CNET_set_handler(EV_DEBUG1,show_table, 0));
    CHECK(CNET_set_debug_string( EV_DEBUG1, "NL info"));

    //init the network layer
    init_network();
	//if(nodeinfo.nodenumber == 0)
	CNET_enable_application(ALLNODES);
}
