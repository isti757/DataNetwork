#include "cnet.h"
#include <stdio.h>
#include <string.h>
#include "datalink_layer.h"
#include "network_layer.h"
#include "datalink_container.h"

//A frame output queue
QUEUE dtg_container_queue;
//timer for flushing output queue
CnetTimerID flush_queue_timer = NULLTIMER;
//an array of tracking timers
CnetTimerID datalink_timers[MAX_LINKS_COUNT];


void flush_queue(CnetEvent ev, CnetTimerID t1, CnetData data);

// read a frame from physical layer
void read_datalink(CnetEvent event, CnetTimerID timer, CnetData data) {
	int link;
	DATAGRAM dtg;
	size_t len = sizeof(DATAGRAM) + 2*MAX_MESSAGE_SIZE;
	//read a datagram from the datalink layer
	CHECK(CNET_read_physical(&link, (char *)&dtg, &len));
	printf("We received on datalink %d bytes\n",len);
	//write the incoming datagram to network layer
	printf("Received checksum=%d\n",dtg.checksum);
	int32_t checksum = dtg.checksum;
	dtg.checksum = 0;
	len = DATAGRAM_SIZE(dtg);
	int checksum_to_compare = CNET_ccitt((unsigned char *) &dtg, len);
	if (checksum_to_compare != checksum) {
		printf("BAD checksum - frame ignored\n");
		return; // bad checksum, ignore frame
	}
	read_network(link, dtg);
}
//-----------------------------------------------------------------------------
// write packet to the link
void write_datalink(int link, char *packet, uint32_t length) {
	 //write Frame to on target link
	printf("Written to datalink queue %d\n",length);
	DTG_CONTAINER container;
	container.len = length;
	container.link = link;
	size_t packet_length = length;
	memcpy(&container.packet,packet,packet_length);
	size_t container_length = DTG_CONTAINER_SIZE(container);
	queue_add(dtg_container_queue,&container,container_length);
}


void flush_queue(CnetEvent ev, CnetTimerID t1, CnetData data) {
	printf("Number in datagram queue %d\n",queue_nitems(dtg_container_queue));
	if (queue_nitems(dtg_container_queue) > 0) {
		size_t containter_len;
		DTG_CONTAINER * dtg_container = queue_remove(dtg_container_queue, &containter_len);
		printf("Flushing datalink queue\n");
		//if this link is free now
		if (datalink_timers[dtg_container->link] == NULLTIMER) {
			printf("Link is not busy %d\n",dtg_container->link);
			//take length and link id
			size_t datagram_length = dtg_container->len;
			int link = dtg_container->link;

			//compute timeout for the link
			CnetTime timeout = 1 + 1000000.0 * ((float) ((datagram_length) * 8.1)
					/ (float) linkinfo[link].bandwidth);
			CHECK(CNET_write_physical(link, (char *)dtg_container->packet, &datagram_length));
			//start timer for the link
			datalink_timers[link] = CNET_start_timer(EV_DATALINK_FREE,timeout,link);

		} else {
			printf("Link is busy %d\n",dtg_container->link);
			//put packet to queue again
			DTG_CONTAINER copy_dtg;
			copy_dtg.len = dtg_container->len;
			copy_dtg.link = dtg_container->link;
			size_t copy_len = dtg_container->len;
			memcpy(copy_dtg.packet,dtg_container->packet,copy_len);
			size_t container_length = DTG_CONTAINER_SIZE(copy_dtg);
			queue_add(dtg_container_queue,&copy_dtg,container_length);
		}
		free((DATAGRAM*)dtg_container);
	}
	CnetTime timeout_flush = 100000;
	flush_queue_timer = NULLTIMER;
	flush_queue_timer = CNET_start_timer(EV_DATALINK_FLUSH, timeout_flush, (CnetData) -1);
}
//stop link timer after sending a datagram
void stop_link_timer(CnetEvent ev, CnetTimerID t1, CnetData data) {
	datalink_timers[(int)data] = NULLTIMER;
}
//-----------------------------------------------------------------------------
// initialization: called by reboot_node
void init_datalink() {
	//set event handler for physical ready event
	CHECK(CNET_set_handler( EV_PHYSICALREADY, read_datalink, 0));
	CHECK(CNET_set_handler( EV_DATALINK_FREE, stop_link_timer, 0));
	//init output queue
	dtg_container_queue = queue_new();

	CHECK(CNET_set_handler( EV_DATALINK_FLUSH, flush_queue, 0));
	CnetTime timeout = 10000;
	flush_queue_timer = CNET_start_timer(EV_DATALINK_FLUSH, timeout, (CnetData) -1);
	for (int i=1;i<=nodeinfo.nlinks;i++) {
		datalink_timers[i] = NULLTIMER;
	}
}
//-----------------------------------------------------------------------------

