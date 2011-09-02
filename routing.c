/*
 * routing.c
 *
 *  Created on: 02.08.2011
 *      Author: kirill
 */
#include <unistd.h>

#include "routing.h"

//-----------------------------------------------------------------------------
// the routing table
ROUTE_TABLE *route_table;
static int route_table_size;
//-----------------------------------------------------------------------------
// the history table
HISTORY_TABLE *history_table;
static int history_table_size;
//-----------------------------------------------------------------------------
// route request counter
static int route_req_id;
//-----------------------------------------------------------------------------
// an array of pending route requests
CnetTimerID pending_route_requests[MAX_HOSTS_NUMBER];
//-----------------------------------------------------------------------------
// find the address in the routing table
static int find_address(CnetAddr address) {
    for (int t = 0; t < route_table_size; ++t)
        if (route_table[t].address == address)
            return (t);

    route_table = (ROUTE_TABLE *) realloc((char *) route_table,
            (route_table_size + 1) * sizeof(ROUTE_TABLE));

    route_table[route_table_size].address = address;
    route_table[route_table_size].minhops = 0;
    route_table[route_table_size].minhop_link = 0;
    route_table[route_table_size].total_delay = 0;
    route_table[route_table_size].min_mtu = 0;

    return (route_table_size++);
}
//-----------------------------------------------------------------------------
// check if address exists in the routing table
int route_exists(CnetAddr addr) {
    for (int t = 0; t < route_table_size; ++t) {
        if (route_table[t].address == addr) {
            return 1;
        }
    }
    return 0;
}
//-----------------------------------------------------------------------------
// find a link to send datagram
static int whichlink(CnetAddr address) {
    int link = route_table[find_address(address)].minhop_link;
    return link;
}
//-----------------------------------------------------------------------------
// learn routing table TODO what about best route?
void learn_route_table(CnetAddr address, int hops, int link, int mtu, CnetTime total_delay) {
    int t = find_address(address);
    //if(route_table[t].minhops >= hops) {
    route_table[t].minhops = hops;
    route_table[t].minhop_link = link;
    route_table[t].min_mtu = mtu;
    route_table[t].total_delay = total_delay;
    //}
}
//-----------------------------------------------------------------------------
// take a packet from the transport layer and send it to the destination
void route(DATAGRAM dtg) {
    N_DEBUG1("Perform routing to %d\n", dtg.dest);
    int link = get_next_link_for_dest(dtg.dest);
    if (link == -1) {
        N_DEBUG1("in route: no address for %d sending request\n", dtg.dest);
    } else {
        N_DEBUG2("in route: send to link %d\n to address=%d\n", link, dtg.dest);
        send_packet_to_link(link, dtg);
    }
}
//-----------------------------------------------------------------------------
void show_table(CnetEvent ev, CnetTimerID timer, CnetData data) {

    //CNET_clear();
    N_DEBUG1("table_size=%d\n", route_table_size);
    N_DEBUG5("\n%14s %14s %14s %14s %14s\n", "destination", "min-hops",
            "minhop-link", "mtu", "time");
    for (int t = 0; t < route_table_size; ++t) {
        N_DEBUG5("%14d %14d %14ld %14ld %14llu\n", (int) route_table[t].address,
                route_table[t].minhops, route_table[t].minhop_link,
                route_table[t].min_mtu, route_table[t].total_delay);
    }

    /*printf("history_table_size=%d\n",history_table_size);
     printf("%14s %14s\n","source","id");
     for (t = 0; t < history_table_size; ++t) {
     printf("%14d %14d\n", (int) history_table[t].source,history_table[t].req_id);
     }*/

    /* printf("reverse_table_size=%d\n",reverse_table_size);
     printf("%14s %14s\n","source","link");
     for (t = 0; t < reverse_table_size; ++t) {
     printf("%14d %14d\n", (int) reverse_route[t].source,reverse_route[t].link);
     }*/
}
//-----------------------------------------------------------------------------
//find an item in the local history
int find_local_history(CnetAddr dest, int req_id) {
    for (int t = 0; t < history_table_size; ++t) {
        if ((history_table[t].source == dest) &&
            (history_table[t].req_id == req_id)) {
            return t;
        }
    }
    return -1;
}
//-----------------------------------------------------------------------------
// insert an item in the local history
int insert_local_history(CnetAddr addr, int req_id) {
    history_table = (HISTORY_TABLE *) realloc((char *) history_table,
            (history_table_size + 1) * sizeof(HISTORY_TABLE));
    history_table[history_table_size].source = addr;
    history_table[history_table_size].req_id = req_id;
    return (history_table_size++);
}
//-----------------------------------------------------------------------------
// process a routing datagram
void do_routing(int link, DATAGRAM datagram) {
    ROUTE_PACKET r_packet;
    size_t datagram_length = datagram.length;
    memcpy(&r_packet, &datagram.payload, datagram_length);

    switch (r_packet.type) {
    case RREQ:
        N_DEBUG2("Received rreq on %d link=%d\n", nodeinfo.address, link);
        N_DEBUG1("hop count = %d\n", r_packet.hop_count);
        // check local history if we already processed this RREQ
        if (find_local_history(r_packet.source, r_packet.req_id) != -1) {
            break;
        }
        N_DEBUG("no entry in local history\n");
        // insert into local history table
        insert_local_history(r_packet.source, r_packet.req_id);
        //set up mtu
        if (r_packet.min_mtu==-1) {
        	r_packet.min_mtu = linkinfo[link].mtu;
        } else {
        	//if this link mtu is less then set up the new mtu
        	if (linkinfo[link].mtu < r_packet.min_mtu) {
        		r_packet.min_mtu = linkinfo[link].mtu;
        	}
        }
        //set up the total delay
        r_packet.total_delay += linkinfo[link].propagationdelay;
        N_DEBUG1("Total delay=%d\n",r_packet.total_delay);

        //check if the route exists and learn the table
        if (route_exists(r_packet.source) == 0) {
            learn_route_table(r_packet.source, r_packet.hop_count, link,
                    r_packet.min_mtu, r_packet.total_delay);
        }
        if (nodeinfo.address == r_packet.dest) {
            // create RREP
            N_DEBUG2("Sending RREP from %d to link %d\n", nodeinfo.address, link);
            ROUTE_PACKET reply_packet;
            reply_packet.type = RREP;
            reply_packet.source = r_packet.source;
            reply_packet.dest = r_packet.dest;
            reply_packet.hop_count = 0;
            reply_packet.total_delay = linkinfo[link].propagationdelay;
            reply_packet.min_mtu = linkinfo[link].mtu;

            uint16_t r_size = ROUTE_PACKET_SIZE(reply_packet);
            DATAGRAM reply_datagram = alloc_datagram(__ROUTING__, r_packet.source, r_packet.dest, (char*) &reply_packet, r_size);
            send_packet_to_link(link, reply_datagram);
            break;
        }
        // the receiver doesn't know
        // increment hop count
        r_packet.hop_count = r_packet.hop_count + 1;
        // rebroadcast the message
        uint16_t request_size = ROUTE_PACKET_SIZE(r_packet);
        DATAGRAM r_datagram = alloc_datagram(__ROUTING__, r_packet.source, r_packet.dest, (char*) &r_packet, request_size);

        // broadcast if no route is known
        if (route_exists(r_packet.dest) == 0) {
            broadcast_packet(r_datagram, link);
        } else {
            //send to dest directly
            int dest_link = get_next_link_for_dest(r_packet.dest);
            send_packet_to_link(dest_link, r_datagram);
        }
        break;
    case RREP:
        N_DEBUG2("Received rrep on %d link=%d\n", nodeinfo.address, link);
        N_DEBUG1("hop count = %d\n", r_packet.hop_count);
        r_packet.hop_count = r_packet.hop_count + 1;
        if (route_exists(r_packet.dest) == 0) {
            learn_route_table(r_packet.dest, r_packet.hop_count, link,
                              r_packet.min_mtu, r_packet.total_delay); //TODO usec!
        }
        // if the packet hasn't reach its destination
        if (nodeinfo.address != r_packet.source) {
            N_DEBUG("Send rrep to reverse route\n");
            int reverse_route_link_id = get_next_link_for_dest(r_packet.source);
            if (reverse_route_link_id == -1) {
                N_DEBUG("Link for reverse routing RREP==-1");
                break;
            }
            // set up propagation delay and mtu
            r_packet.total_delay = r_packet.total_delay
                    + linkinfo[reverse_route_link_id].propagationdelay;
            if (r_packet.min_mtu > linkinfo[reverse_route_link_id].mtu) {
                r_packet.min_mtu = linkinfo[reverse_route_link_id].mtu;
            }
            uint16_t reply_size = ROUTE_PACKET_SIZE(r_packet);
            DATAGRAM reply_datagram = alloc_datagram(__ROUTING__, r_packet.source,
                                                     r_packet.dest, (char*) &r_packet,
                                                     reply_size);

            send_packet_to_link(reverse_route_link_id, reply_datagram);
        }
        break;
    }
}
//-----------------------------------------------------------------------------
// detect a link for outcoming message
int get_next_link_for_dest(CnetAddr destaddr) {
    // address not exists yet
    if (route_exists(destaddr) == 0) {
        N_DEBUG("the address is not in route table\n");
        return -1;
    } else {
        N_DEBUG1("the address is in route table link=%d\n",
                whichlink(destaddr));
        return whichlink(destaddr);
    }
}
//-----------------------------------------------------------------------------
// send a route request
void send_route_request(CnetAddr destaddr) {
    if (pending_route_requests[destaddr] == NULLTIMER) {
        ROUTE_PACKET req_packet;
        req_packet.type = RREQ;
        req_packet.source = nodeinfo.address;
        req_packet.dest = destaddr;
        req_packet.hop_count = 0;
        req_packet.total_delay = 0;
        req_packet.req_id = route_req_id;
        req_packet.min_mtu = -1;
        route_req_id++;
        uint16_t request_size = sizeof(req_packet);
        DATAGRAM r_packet = alloc_datagram(__ROUTING__, nodeinfo.address,
                                            destaddr, (char*) &req_packet,
                                            request_size);

        N_DEBUG1("sending rreq for %d\n", destaddr);

        // start timer for pending route request
        pending_route_requests[destaddr] = CNET_start_timer(
                EV_ROUTE_PENDING_TIMER, ROUTE_REQUEST_TIMEOUT, destaddr);
        broadcast_packet(r_packet, -1);
    }
}
//-----------------------------------------------------------------------------
// detect fragmentation size for the specified destination
int get_mtu(CnetAddr address) {
    if (route_exists(address) == 1) {
        int t = find_address(address);
        return route_table[t].min_mtu - PACKET_HEADER_SIZE - DATAGRAM_HEADER_SIZE;
    } else {
        send_route_request(address);
        return -1;
    }
}
//-----------------------------------------------------------------------------
void route_request_resend(CnetEvent ev, CnetTimerID timer, CnetData data) {
    int address = (int) data;
    N_DEBUG1("Checking route request for %d\n", address);
    if (route_exists(address) == 0) {
        pending_route_requests[address] = NULLTIMER;
        N_DEBUG1("Resending route request for %d\n", address);
        send_route_request(address);
    }
}
//-----------------------------------------------------------------------------
//  initialize the routing protocol
void init_routing() {
    route_table = (ROUTE_TABLE *) malloc(sizeof(ROUTE_TABLE));
    route_table_size = 0;

    history_table = (HISTORY_TABLE*) malloc(sizeof(HISTORY_TABLE));
    history_table_size = 0;

    route_req_id = 0;

    CHECK(CNET_set_handler(EV_DEBUG1,show_table, 0));
    CHECK(CNET_set_debug_string( EV_DEBUG1, "NL info"));

    // init route requests
    for (int i = 0; i < MAX_HOSTS_NUMBER; i++) {
        pending_route_requests[i] = NULLTIMER;
    }
    // register handler for resending route requests
    CHECK(CNET_set_handler(EV_ROUTE_PENDING_TIMER,route_request_resend, 0));
}
//-----------------------------------------------------------------------------
void shutdown_routing() {
    free(route_table);
    free(history_table);
}
//-----------------------------------------------------------------------------

