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

//-----------------------------------------------------------------------------
/* initialization: called by reboot_node */
void init_datalink() {
	/* set event handler for physical ready event */
	CHECK(CNET_set_handler( EV_PHYSICALREADY, read_datalink, 0));
}
//-----------------------------------------------------------------------------
/* read a frame from physical layer */
void read_datalink(CnetEvent event, CnetTimerID timer, CnetData data) {
	int link;
	DATAGRAM dtg;
	size_t len = sizeof(DATAGRAM) + MAX_MESSAGE_SIZE;
	//read a datagram from the datalink layer
	CHECK(CNET_read_physical(&link, (char *)&dtg, &len));
	//write the incoming datagram to network layer
	int checksum = dtg.checksum;
	dtg.checksum = 0;
	len = DATAGRAM_SIZE(dtg);
	int checksum_to_compare = CNET_ccitt((unsigned char *) &dtg, len);
	if (checksum_to_compare != checksum) {
		printf("BAD checksum - frame ignored\n");
		return; // bad checksum, ignore frame
	}
	read_network(link, dtg, len);
}
//-----------------------------------------------------------------------------
/* write packet to the link */
void write_datalink(int link, char *packet, size_t length) {
	/* write Frame to on target link */
	CHECK(CNET_write_physical(link, (char *)packet, &length));
}

