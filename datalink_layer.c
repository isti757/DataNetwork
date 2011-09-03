/*
 * datalink_layer.h
 *
 *  Created on: 30.07.2011
 *      Author: kirill
 */

#include "cnet.h"

#include <stdio.h>
#include <string.h>

#include "debug.h"
#include "datalink_layer.h"
#include "network_layer.h"

//-----------------------------------------------------------------------------
// an array of link timers
CnetTimerID datalink_timers[MAX_LINKS_COUNT];
// an array of output queues for each link
QUEUE output_queues[MAX_LINKS_COUNT];
//-----------------------------------------------------------------------------
// read a frame from physical layer
void read_datalink(CnetEvent event, CnetTimerID timer, CnetData data) {
    //read a datagram from the datalink layer
    int link;
    DL_FRAME frame;
    size_t len = sizeof(DATAGRAM) + 2*MAX_MESSAGE_SIZE;
    CHECK(CNET_read_physical(&link, (char *)&frame, &len));
    D_DEBUG1("We received on datalink %d bytes\n", len);

    // a datagram to network layer
    read_network(link, len, (char*) frame.data);
}
//-----------------------------------------------------------------------------
// write a frame to the link
void write_datalink(int link, char *datagram, uint32_t length) {
    D_DEBUG1("Written to datalink queue %d\n", length);
    DTG_CONTAINER container;
    container.len = length;
    container.link = link;

    size_t datagram_length = length;
    memcpy(&container.data, datagram, datagram_length);

    // check if timer is null to avoid polling
    if (datalink_timers[link] == NULLTIMER) {
        CnetTime timeout_flush = 1;
        datalink_timers[link] = CNET_start_timer(EV_DATALINK_FLUSH, timeout_flush, link);
    }

    // add to the link queue
    size_t container_length = DTG_CONTAINER_SIZE(container);
    queue_add(output_queues[link], &container, container_length);
}
//-----------------------------------------------------------------------------
// flush a queue
void flush_datalink_queue(CnetEvent ev, CnetTimerID t1, CnetData data) {
    int current_link = (int) data;
    D_DEBUG2("Flushing - Number in datagram queue %d link_id=%d\n",
            queue_nitems(output_queues[current_link]), current_link);
    if (queue_nitems(output_queues[current_link]) > 0) {
        // take first datalink frame
        size_t containter_len;
        DTG_CONTAINER * dtg_container = queue_remove(output_queues[current_link], &containter_len);

        // write datalink frame to the link
        DL_FRAME frame;
        int link = dtg_container->link;
        size_t datagram_length = dtg_container->len;
        memcpy(&frame.data, dtg_container->data, datagram_length);
        /*if (linkinfo[link].mtu < datagram_length) {
        	DATAGRAM dtg;
        	memcpy(&dtg,&frame.data,datagram_length);
        	fprintf(stderr, "Node:%d, mtu:%d, datagram to:%d, from: %d, decl.mtu:%d,length: %d\n", nodeinfo.address,linkinfo[link].mtu,dtg.dest,dtg.src,dtg.declared_mtu,datagram_length);
        }*/
        CHECK(CNET_write_physical(link, (char *)&frame, &datagram_length));

        //compute timeout for the link
        double bandwidth = linkinfo[link].bandwidth;
        CnetTime timeout = 1+8000005.0*(datagram_length / bandwidth);
        datalink_timers[link] = CNET_start_timer(EV_DATALINK_FLUSH, timeout, current_link);
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
    CHECK(CNET_set_handler( EV_DATALINK_FLUSH, flush_datalink_queue, 0));

    for (int i = 1; i <= nodeinfo.nlinks; i++) {
        datalink_timers[i] = NULLTIMER;
        output_queues[i] = queue_new();
    }
}
//-----------------------------------------------------------------------------
void shutdown_datalink() {
    for (int i = 1; i <= nodeinfo.nlinks; i++) {
        fprintf(stderr, "\tlink %d - datalink queue: %d packets\n", i, queue_nitems(output_queues[i]));
        queue_free(output_queues[i]);
    }
}
//-----------------------------------------------------------------------------
