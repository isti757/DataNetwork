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


QUEUE frame_queue;
QUEUE link_queue;
QUEUE length_queue;
CnetTimerID flush_queue_timer = NULLTIMER;

void flush_queue(CnetEvent ev, CnetTimerID t1, CnetData data);
//-----------------------------------------------------------------------------
/* initialization: called by reboot_node */
void init_datalink() {
	/* set event handler for physical ready event */
	CHECK(CNET_set_handler( EV_PHYSICALREADY, read_datalink, 0));
	frame_queue = queue_new();
	link_queue = queue_new();
	length_queue = queue_new();

	CHECK(CNET_set_handler( EV_TIMER6, flush_queue, 0));
	CnetTime timeout = 10000;
	flush_queue_timer = CNET_start_timer(EV_TIMER6, timeout, (CnetData) -1);
}
//-----------------------------------------------------------------------------
/* read a frame from physical layer */
void read_datalink(CnetEvent event, CnetTimerID timer, CnetData data) {
	int link;
	DATAGRAM dtg;
	size_t len = sizeof(DATAGRAM) + MAX_MESSAGE_SIZE;
	//read a datagram from the datalink layer
	CHECK(CNET_read_physical(&link, (char *)&dtg, &len));
	printf("Receive on datalink %d\n",len);
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
void write_datalink(int link, char *packet, uint32_t length) {
	/* write Frame to on target link */
	printf("Written to datalink queue %d\n",length);
	queue_add(frame_queue,packet,length);
	queue_add(link_queue,&link,sizeof(int));
	queue_add(length_queue,&length,sizeof(uint32_t));
	//CHECK(CNET_write_physical(link, (char *)packet, &length));
}


void flush_queue(CnetEvent ev, CnetTimerID t1, CnetData data) {
	printf("Number in datagram queue %d\n",queue_nitems(frame_queue));
	if (queue_nitems(frame_queue) > 0) {
		printf("Flushing datalink queue\n");
		size_t len_len;
		size_t* length = queue_remove(length_queue,&len_len);
		size_t data_len;
		char * datagram = queue_remove(frame_queue, &data_len);
		size_t link_len;
		int* link = queue_remove(link_queue,&link_len);
		CnetTime timeout = 1 + 1000000.0 * ((float) ((*length) * 8.1)/ (float) linkinfo[*link].bandwidth);
		size_t l = *length;
		CHECK(CNET_write_physical(*link, (char *)datagram, &l));
		flush_queue_timer = NULLTIMER;
		flush_queue_timer = CNET_start_timer(EV_TIMER6, timeout, (CnetData) -1);
	} else {
		CnetTime timeout = 50000;
		flush_queue_timer = NULLTIMER;
		flush_queue_timer = CNET_start_timer(EV_TIMER6, timeout, (CnetData) -1);
	}
}
