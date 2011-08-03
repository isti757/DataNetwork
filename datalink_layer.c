/*
 * datalink_layer.c
 *
 *  Created on: Jul 30, 2011
 *      Author: isti
 */
#include "cnet.h"
#include <stdio.h>
#include <string.h>
#include "datalink_layer.h"
#include "network_layer.h"


/* initialization: called by reboot_node */

void init_datalink() {

  /* set event handler for physical ready event */
  CHECK(CNET_set_handler( EV_PHYSICALREADY, read_datalink, 0));
}



/* frame from physical layer */

void read_datalink(CnetEvent event, CnetTimerID timer, CnetData data)
{
	int link;
	size_t len;
	DATAGRAM dtg;
	len = sizeof(DATAGRAM)+2*MAX_MESSAGE_SIZE;
	//read a datagram from the datalink layer
	CHECK(CNET_read_physical(&link, (char *)&dtg, &len));
	//write the incoming datagram to network layer

	read_network(link, dtg, len);

}



/* packet from network layer */

void write_datalink(int link, char *packet, size_t length)
{
    /* write Frame to on target link */
  CHECK(CNET_write_physical(link, (char *)packet, &length));

}

