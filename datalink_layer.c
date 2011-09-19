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
    int link;
    DL_FRAME frame;
    size_t len = DATAGRAM_HEADER_SIZE + DL_FRAME_HEADER_SIZE + 2*MAX_MESSAGE_SIZE;
    CHECK(CNET_read_physical(&link, (char *)&frame, &len));
    uint32_t checksum = frame.checksum;
    //D_DEBUG3("We received on link %d %d bytes checksum=%lu\n",link ,len,checksum);
    size_t dtg_len = len - DL_FRAME_HEADER_SIZE;
    uint32_t checksum_to_compare = CNET_crc32((unsigned char *)&frame.data, dtg_len);
    if (checksum_to_compare != checksum) {
        //D_DEBUG("BAD checksum - ignored\n");
        return; // bad checksum, ignore frame
    }
    //read a datagram to network layer
    read_network(link, dtg_len, (char*) frame.data);
}
//-----------------------------------------------------------------------------
// write a frame to the link
void write_datalink(int link, char *datagram, uint32_t checksum, uint32_t length) {
    //D_DEBUG2("Written to datalink queue %d checksum %lu\n", length,checksum);
    DTG_CONTAINER container;
    container.len = length;
    container.link = link;
    container.checksum = checksum;
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
    //D_DEBUG2("Flushing - Number in datagram queue %d link_id=%d\n",
    //        queue_nitems(output_queues[current_link]), current_link);
    if (queue_nitems(output_queues[current_link]) > 0) {
        // take a first datalink frame
        size_t containter_len;
        DTG_CONTAINER * dtg_container = queue_remove(output_queues[current_link], &containter_len);
        // write datalink frame to the link
        DL_FRAME frame;
        int link = dtg_container->link;
        size_t datagram_length = dtg_container->len;
        frame.checksum = dtg_container->checksum;
        //D_DEBUG3("Flushed to link %d %d bytes checksum %lu\n",link,datagram_length,frame.checksum);
        memcpy(&frame.data, dtg_container->data, datagram_length);
        size_t frame_length = datagram_length + DL_FRAME_HEADER_SIZE;
        if (frame_length > linkinfo[link].mtu) {
            DATAGRAM dtg;
            memcpy(&dtg,&frame.data,datagram_length);
            fprintf(stderr,"BADSIZE: host %d, actual mtu:%d,decl mtu:%d, from %d to %d lenght %d req_id %d\n",
                    nodeinfo.address,linkinfo[link].mtu,dtg.declared_mtu,dtg.src,dtg.dest,frame_length,
                    dtg.req_id);
        }
        CHECK(CNET_write_physical(link, (char *)&frame, &frame_length));
        //compute timeout for the link
        double bandwidth = linkinfo[link].bandwidth;
        CnetTime timeout = 1+8000005.0*(frame_length / bandwidth);
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
        if(queue_nitems(output_queues[i]) > 1000) {
//            char filename [] = "datalink___";
//            sprintf(filename+8, "%3d", nodeinfo.address);
//            FILE* datalink_file = fopen(filename, "a");

            while(queue_nitems(output_queues[i]) != 0) {
                size_t containter_len;
                DTG_CONTAINER * dtg_container = queue_remove(output_queues[i], &containter_len);
                DATAGRAM dtgr; size_t len = dtg_container->len;
                memcpy(&dtgr, dtg_container->data, len);
                //fprintf(datalink_file, "src: %u dest: %u len: %d kind: %u\n", dtgr.src, dtgr.dest, dtgr.length, dtgr.kind);

                free(dtg_container);
            }

            //fclose(datalink_file);
        }
        queue_free(output_queues[i]);
    }
}
//-----------------------------------------------------------------------------
