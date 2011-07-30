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

	MSG msg;
	int msg_size = sizeof(MSG);
	CHECK(CNET_read_application(&destaddr, (char *)&msg, (size_t*)&msg_size));

	// pass message to transport
	PACKET pkt;
	pkt.data = msg;
	pkt.len = msg_size;

	read_transport(destaddr, pkt);
}
//-----------------------------------------------------------------------------
static EVENT_HANDLER(physical_ready)
{
	int link;
	size_t len;
	DATAGRAM dtg;

	CHECK(CNET_read_physical(&link, (char *)&dtg, &len));

	// checksum check
	// maybe send NACK

	write_network(link, dtg);
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
//
//	CHECK(CNET_set_debug_string( EV_DEBUG0, "State"));

	//if(nodeinfo.nodenumber == 0)
	CNET_enable_application(ALLNODES);
}
