/*
 * datalink_layer.c
 *
 *  Created on: Jul 30, 2011
 *      Author: isti
 */

#include <stdio.h>
#include <string.h>

#include "cnet.h"
#include "datalink_layer.h"
#include "network_layer.h"
#include "datalink_container.h"

//-----------------------------------------------------------------------------
// the datagram dispatch queue
QUEUE dtg_container_queue;
//-----------------------------------------------------------------------------
// timer for determining state of the link
CnetTimerID link_busy_timer;
//-----------------------------------------------------------------------------
void flush_datalink_queue(CnetEvent ev, CnetTimerID t1, CnetData data) {
    if (queue_nitems(dtg_container_queue) > 0) {
        size_t containter_len;
        DTG_CONTAINER * dtg_container = queue_remove(dtg_container_queue,
                &containter_len);

        size_t datagram_length = dtg_container->len;
        int link = dtg_container->link;

        // compute timeout for sending the next datagram
        CnetTime timeout = 1 + 1000000.0 * ((double) ((datagram_length) * 8.0)
                / (double) linkinfo[link].bandwidth);
        CHECK(CNET_write_physical(link, (char *)dtg_container->datagram, &datagram_length));

        //start timer for the link
        link_busy_timer = CNET_start_timer(EV_DATALINK_FREE, timeout, link);

        free((DATAGRAM*) dtg_container);
    } else
        link_busy_timer = NULLTIMER;
}
//-----------------------------------------------------------------------------
// read a frame from physical layer
void read_datalink(CnetEvent event, CnetTimerID timer, CnetData data) {
    int link;
    DATAGRAM dtg;
    size_t len = sizeof(DATAGRAM) + MAX_MESSAGE_SIZE;

    //read a datagram from the datalink layer
    CHECK(CNET_read_physical(&link, (char *)&dtg, &len));

    // check that the datagram is not corrupted
    uint16_t checksum = dtg.checksum;
    dtg.checksum = 0;
    if (CNET_ccitt((unsigned char *) &dtg, len) != checksum) {
        printf("b\t\tBAD checksum\n");
        return; /* bad checksum, ignore frame */
    }

    read_network(link, dtg, len);
}
//-----------------------------------------------------------------------------
// write packet to the link
void write_datalink(int link, DATAGRAM dtg) {
    // write datagram to the target link
    DTG_CONTAINER container;
    container.len = DATAGRAM_SIZE(dtg);
    container.link = link;

    dtg.checksum = 0;
    dtg.checksum = CNET_ccitt((unsigned char*)&dtg, DATAGRAM_SIZE(dtg));

    size_t packet_length = container.len;
    memcpy(&container.datagram, &dtg, packet_length);
    size_t container_length = DTG_CONTAINER_SIZE(container);

    if (link_busy_timer == NULLTIMER) {
        CnetTime timeout_flush = 1;
        link_busy_timer = CNET_start_timer(EV_DATALINK_FREE, timeout_flush, 0);
    }
    queue_add(dtg_container_queue, &container, container_length);
}
//-----------------------------------------------------------------------------
// initialization: called by reboot_node
void init_datalink() {
    // set event handler for physical ready event
    CHECK(CNET_set_handler( EV_PHYSICALREADY, read_datalink, 0));
    CHECK(CNET_set_handler( EV_DATALINK_FREE, flush_datalink_queue, 0));

    // initialize output queue
    dtg_container_queue = queue_new();

    link_busy_timer = NULLTIMER;
}
//-----------------------------------------------------------------------------
void shutdown_datalink() {
    queue_free(dtg_container_queue);
}
//-----------------------------------------------------------------------------
void print_datalink_layer() {
    printf("\tdatalink queue size\t = %d\n", queue_nitems(dtg_container_queue));
}
//-----------------------------------------------------------------------------
