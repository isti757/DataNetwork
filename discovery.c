#include <cnet.h>
#include <malloc.h>
#include <string.h>
#include "log.h"
#include "discovery.h"
#include "routing.h"
//-----------------------------------------------------------------------------
static int response_status[MAX_LINKS_COUNT];
//-----------------------------------------------------------------------------
// initialize discovery protocol
void init_discovery() {
    // establish the timer event handler
    CHECK(CNET_set_handler(EV_DISCOVERY_TIMER, discovery_timer_handler, 0));

    // initialize the response status
    for (int i = 1; i <= nodeinfo.nlinks; i++)
        response_status[i] = 0;

    CNET_start_timer(EV_DISCOVERY_TIMER, DISCOVERY_START_UP_TIME, POLLTIMER);
}
//-----------------------------------------------------------------------------
/* process a discovery protocol packet */
void do_discovery(int link, DATAGRAM datagram) {
    DISCOVERY_PACKET p;
    size_t datagram_length = datagram.length;
    memcpy(&p, &datagram.payload, datagram_length);

    // process request/response
    switch (p.type) {
    case Who_R_U:
        // request that we identify ourselves - send I_Am reply
    	;
        DISCOVERY_PACKET dpkt;
        dpkt.type = I_AM;
        dpkt.address = nodeinfo.address; // my address
        dpkt.delay = nodeinfo.time_in_usec-p.delay;    // return sender's time stamp

        // initialize and send a packet
        uint16_t packet_size = DISCOVERY_PACKET_SIZE(dpkt);
        DATAGRAM np = alloc_datagram(__DISCOVER__, nodeinfo.address, p.address, (char *) &dpkt, packet_size);
        send_packet_to_link(link, np);
        break;
    case I_AM:
        // response to our Who_R_U query
        N_DEBUG("learning table...\n");
        learn_route_table(p.address, 0, link, linkinfo[link].mtu, p.delay,-1);
        fprintf(routing_log,"Poll response from link:%d addr:%d\n", link, p.address);
        response_status[link] = 1;
        break;
    }
}
//-----------------------------------------------------------------------------
// poll our neighbor
void pollNeighbor(int link) {
    // send a Poll message
    DISCOVERY_PACKET p;
    p.type = Who_R_U;                    //the request
    p.address = nodeinfo.address;        //my address
    p.delay = nodeinfo.time_in_usec; //time we send query

    uint16_t packet_size = DISCOVERY_PACKET_SIZE(p);
    DATAGRAM np = alloc_datagram(__DISCOVER__, nodeinfo.address, -1, (char *) &p, packet_size);

    send_packet_to_link(link, np);
}
//-----------------------------------------------------------------------------
// discovery timer event handler
void discovery_timer_handler(CnetEvent ev, CnetTimerID timer, CnetData data) {
    switch ((int) data) {
    case POLLTIMER:
        // time to poll neighbors again
        N_DEBUG("Start polling..\n");
        for (int i = 1; i <= nodeinfo.nlinks; i++) {
            pollNeighbor(i);
            pollNeighbor(i);
        }
        // start poll fail timer for this poll period
        CNET_start_timer(EV_DISCOVERY_TIMER, DISCOVERY_TIME_OUT, POLLRESPONSETIMER);
        break;
    case POLLRESPONSETIMER:
        N_DEBUG("Checking for polling responses\n");
        int run_timer = 0;
        for (int i = 1; i <= nodeinfo.nlinks; i++) {
            if (response_status[i] == 0) {
                pollNeighbor(i);
                run_timer = 1;
            }
        }
        if (run_timer == 1) {
            CNET_start_timer(EV_DISCOVERY_TIMER, DISCOVERY_TIME_OUT, POLLRESPONSETIMER);
            N_DEBUG("Timer started again\n");
        } else {
            // notify the transport layer
            signal_transport(DISCOVERY_FINISHED, -1);
        }
        break;
    }
}
//-----------------------------------------------------------------------------

