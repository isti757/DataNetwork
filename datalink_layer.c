#include "cnet.h"
#include <stdio.h>
#include <string.h>
#include "datalink_layer.h"
#include "network_layer.h"
#include "datalink_container.h"
#include "dl_frame.h"

//an array of link timers
CnetTimerID datalink_timers[MAX_LINKS_COUNT];
//an array of output queues for each link
QUEUE output_queues[MAX_LINKS_COUNT];


//-----------------------------------------------------------------------------
// read a frame from physical layer
void read_datalink(CnetEvent event, CnetTimerID timer, CnetData data) {
	int link;
	//DATAGRAM dtg;
	DL_FRAME frame;
	size_t len = sizeof(DATAGRAM) + 2*MAX_MESSAGE_SIZE;
	//read a datagram from the datalink layer
	CHECK(CNET_read_physical(&link, (char *)&frame, &len));
	printf("We received on datalink %d bytes\n",len);
	//read network
	read_network(link, len,(char*)frame.data);
}
//-----------------------------------------------------------------------------
// write a frame to the link
void write_datalink(int link, char *datagram, uint32_t length) {
	printf("Written to datalink queue %d\n",length);
	DTG_CONTAINER container;
	container.len = length;
	container.link = link;
	size_t data_length = length;
	memcpy(&container.data,datagram,data_length);
	size_t container_length = DTG_CONTAINER_SIZE(container);

	//check timer
	if (datalink_timers[link] == NULLTIMER) {
		 CnetTime timeout_flush = 1;
		 datalink_timers[link] = CNET_start_timer(EV_DATALINK_FLUSH, timeout_flush, link);
	}
	//add to the link queue
	queue_add(output_queues[link],&container,container_length);

}
//-----------------------------------------------------------------------------
//Flush a queue
void flush_datalink_queue(CnetEvent ev, CnetTimerID t1, CnetData data) {
	int current_link = (int)data;
	printf("Flushing - Number in datagram queue %d link_id=%d\n",queue_nitems(output_queues[current_link]),current_link);
	if (queue_nitems(output_queues[current_link]) > 0) {
		size_t containter_len;
		DTG_CONTAINER * dtg_container = queue_remove(output_queues[current_link], &containter_len);
		size_t datagram_length = dtg_container->len;
		int link = dtg_container->link;

		//compute timeout for the link
		CnetTime timeout = 1 + 1000000.0 * ((double) ((datagram_length) * 8.0)
		                / (double) linkinfo[link].bandwidth);
		DL_FRAME frame;
		memcpy(&frame.data,dtg_container->data,datagram_length);
		CHECK(CNET_write_physical(link, (char *)&frame, &datagram_length));
		datalink_timers[link] = CNET_start_timer(EV_DATALINK_FLUSH,timeout,current_link);
		free((DTG_CONTAINER*) dtg_container);
	} else {
		datalink_timers[current_link] = NULLTIMER;
	}
}
//-----------------------------------------------------------------------------
// initialization: called by reboot_node
void init_datalink() {
	//set event handler for physical ready event
	CHECK(CNET_set_handler( EV_PHYSICALREADY, read_datalink, 0));
	//CHECK(CNET_set_handler( EV_DATALINK_FREE, stop_link_timer, 0));
	//init output queue
	//dtg_container_queue = queue_new();

	CHECK(CNET_set_handler( EV_DATALINK_FLUSH, flush_datalink_queue, 0));
	//CnetTime timeout = 10000;
	//flush_queue_timer = CNET_start_timer(EV_DATALINK_FLUSH, timeout, (CnetData) -1);
	for (int i=1;i<=nodeinfo.nlinks;i++) {
		datalink_timers[i] = NULLTIMER;
		output_queues[i] = queue_new();
	}
}
//-----------------------------------------------------------------------------
void shutdown_datalink() {
	for (int i=1;i<=nodeinfo.nlinks;i++) {
		queue_free(output_queues[i]);
	}
}
