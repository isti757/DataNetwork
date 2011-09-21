/*
 * transport_layer.h
 *
 *  Created on: Aug 30, 2011
 *      Author: kirill
 */
#include <unistd.h>

#include "log.h"
#include "routing.h"
#include "dl_frame.h"
#include "network_layer.h"

//-----------------------------------------------------------------------------
// the routing table
ROUTE_TABLE *route_table;
static int route_table_size;
//-----------------------------------------------------------------------------
// the history table
HISTORY_TABLE *history_table;
static int history_table_size;
//-----------------------------------------------------------------------------
// the reverse route table
REVERSE_ROUTE_TABLE *reverse_route;
int reverse_table_size;
//-----------------------------------------------------------------------------
// the forward route table
FORWARD_ROUTE_TABLE *forward_route;
int forward_table_size;
//-----------------------------------------------------------------------------
// route request counter
static int route_req_id;
//-----------------------------------------------------------------------------
// an array of pending route requests
CnetTimerID pending_route_requests[MAX_HOSTS_NUMBER];
//an array of the pending route requests ttl
uint8_t pending_route_request_ttl[MAX_HOSTS_NUMBER];
//-----------------------------------------------------------------------------
// an array of route notifications(if 1 then already notified the TL)
uint8_t route_notified[MAX_HOSTS_NUMBER];
// a bitmap to find out if a route is discovered
uint8_t routes[MAX_HOSTS_NUMBER];
//-----------------------------------------------------------------------------
// find the address in the routing table
static int find_address(CnetAddr address) {
    for (int t = 0; t < route_table_size; ++t)
        if (route_table[t].address == address)
            return (t);

    size_t to_realloc = (route_table_size + 1) * sizeof(ROUTE_TABLE);
    route_table = realloc((char *) route_table, to_realloc);

    route_table[route_table_size].address = address;
    route_table[route_table_size].minhops = 0;
    route_table[route_table_size].minhop_link = -1;
    route_table[route_table_size].total_delay = 0;
    route_table[route_table_size].min_mtu = 0;
    route_table[route_table_size].req_id = -1;
    return (route_table_size++);
}
//-----------------------------------------------------------------------------
// check if address exists in the routing table
int route_exists(CnetAddr addr) {
    if (routes[addr]==0) {
        return 0;
    } else {
        return 1;
    }
}
//-----------------------------------------------------------------------------
// find a link to send datagram
static int whichlink(CnetAddr address) {
    int link = route_table[find_address(address)].minhop_link;
    return link;
}
//-----------------------------------------------------------------------------
// learn routing table TODO what about the best route?
void learn_route_table(CnetAddr address, int hops, int link, int mtu,
                       CnetTime total_delay, int req_id) {
    if (route_exists(address) == 0) {
        int t = find_address(address);
        route_table[t].minhops = hops;
        route_table[t].minhop_link = link;
        route_table[t].min_mtu = mtu;
        route_table[t].total_delay = total_delay;
        route_table[t].req_id = req_id;
        routes[address] = 1;
    }
}
//-----------------------------------------------------------------------------
// insert an item into the reverse route table
int insert_reverse_route(CnetAddr source,CnetAddr dest, int link, int req_id) {
    size_t to_realloc = (reverse_table_size + 1) * sizeof(REVERSE_ROUTE_TABLE);
    reverse_route = realloc((char *) reverse_route, to_realloc);
    reverse_route[reverse_table_size].source = source;
    reverse_route[reverse_table_size].dest = dest;
    reverse_route[reverse_table_size].reverse_link = link;
    reverse_route[reverse_table_size].req_id = req_id;
    return (reverse_table_size++);
}
//-----------------------------------------------------------------------------
// insert an item into the forward route table
int insert_forward_route(CnetAddr source,CnetAddr dest, int link, int req_id) {
    size_t to_realloc = (forward_table_size+1) * sizeof(FORWARD_ROUTE_TABLE);
    forward_route = realloc((char*)forward_route, to_realloc);
    forward_route[forward_table_size].source = source;
    forward_route[forward_table_size].dest = dest;
    forward_route[forward_table_size].forward_link = link;
    forward_route[forward_table_size].req_id = req_id;
    return (forward_table_size++);
}
//-----------------------------------------------------------------------------
// find a reverse route link
int find_reverse_route(CnetAddr source,CnetAddr dest,int req_id) {
    for (int t = 0; t < reverse_table_size; ++t)
         if (reverse_route[t].source == source && reverse_route[t].dest==dest &&
             reverse_route[t].req_id==req_id)
             return reverse_route[t].reverse_link;
    return -1;
}
//-----------------------------------------------------------------------------
// find a reverse route link
int find_forward_route(CnetAddr source,CnetAddr dest,int req_id) {
    for (int t = 0; t <forward_table_size;++t) {
        if (forward_route[t].source==source && forward_route[t].dest==dest &&
            forward_route[t].req_id==req_id) {
            return forward_route[t].forward_link;
        }
    }
    return -1;
}
//-----------------------------------------------------------------------------
// detect a link for outgoing message
int get_next_link_for_dest(CnetAddr destaddr) {
    // address not exists yet
    if (route_exists(destaddr) == FALSE)
        return -1;
    else
        return whichlink(destaddr);
}
//-----------------------------------------------------------------------------
// find a request id of a discovered route
int get_request_id_for_dest(CnetAddr dest) {
    if (route_exists(dest)) {
        int t = find_address(dest);
        return route_table[t].req_id;
    } else
        abort();
}
//-----------------------------------------------------------------------------
// find a request id of a reverse route
int get_request_id_reverse(CnetAddr source,CnetAddr dest) {
    for (int t = 0; t < reverse_table_size; ++t)
         if (reverse_route[t].source == source && reverse_route[t].dest==dest)
             return reverse_route[t].req_id;
    return -1;
}
//-----------------------------------------------------------------------------
// find a request id of a reverse route by src and dest
int find_reverse_route_no_id(CnetAddr source,CnetAddr dest) {
    for (int t = 0; t < reverse_table_size; ++t)
         if (reverse_route[t].source == source && reverse_route[t].dest==dest)
             return reverse_route[t].reverse_link;
    return -1;
}
//-----------------------------------------------------------------------------
// take a packet from the transport layer and send it to the destination
void route(DATAGRAM dtg) {
    int link = -1;
    // use the route table, direct route
    if (dtg.src == nodeinfo.address) {
        link = get_next_link_for_dest(dtg.dest);
        // the route is discovered by this node
        if (link != -1) {
            dtg.req_id = get_request_id_for_dest(dtg.dest);
        } else {
            // find a reverse route
            link = find_reverse_route_no_id(dtg.dest,dtg.src);
            dtg.req_id = get_request_id_reverse(dtg.dest,dtg.src);
        }
    } else {
        // send to a reverse route or a forward route.The datagram from another src
        if (dtg.req_id < 0) {
            abort();
        }
        link = find_forward_route(dtg.src,dtg.dest,dtg.req_id);
        if (link == -1) {
            link = find_reverse_route(dtg.dest,dtg.src,dtg.req_id);
        }
    }
    if (link < 1)
        abort();
    else
        send_packet_to_link(link, dtg);
}
//-----------------------------------------------------------------------------
// find an item in the local history
int find_local_history(CnetAddr source,CnetAddr dest, int req_id) {
    for (int t = 0; t < history_table_size; ++t) {
        if ((history_table[t].source == source) && (history_table[t].dest==dest) &&
            (history_table[t].req_id == req_id)) {
            return t;
        }
    }
    return -1;
}
//-----------------------------------------------------------------------------
// insert an item in the local history
int insert_local_history(CnetAddr source,CnetAddr dest, int req_id) {
    size_t to_realloc = (history_table_size + 1) * sizeof(HISTORY_TABLE);
    history_table = (HISTORY_TABLE *) realloc((char *) history_table, to_realloc);
    history_table[history_table_size].source = source;
    history_table[history_table_size].req_id = req_id;
    history_table[history_table_size].dest = dest;
    return (history_table_size++);
}
//-----------------------------------------------------------------------------
// check if a host is a neighbour
int is_neighbour(CnetAddr address) {
    for (int t = 0; t < route_table_size; ++t) {
        if (route_table[t].address == address && route_table[t].minhops==0) {
            return 1;
        }
    }
    return 0;
}
//-----------------------------------------------------------------------------
// process a routing datagram
void do_routing(int link, DATAGRAM datagram) {

    ROUTE_PACKET r_packet;
    size_t datagram_length = datagram.length;
    memcpy(&r_packet, &datagram.payload, datagram_length);

    switch (r_packet.type) {
    case RREQ:
        // check local history if we already processed this RREQ
        if (find_local_history(r_packet.source,r_packet.dest, r_packet.req_id) != -1) {
            break;
        }
        // insert into local history table
        insert_local_history(r_packet.source, r_packet.dest, r_packet.req_id);

        // set up mtu and delays
        if (r_packet.min_mtu == 0) {
            r_packet.min_mtu = linkinfo[link].mtu;
        } else {
            if (r_packet.min_mtu > linkinfo[link].mtu) {
                r_packet.min_mtu = linkinfo[link].mtu;
            }
        }
        if (r_packet.total_delay == 0) {
            r_packet.total_delay = linkinfo[link].propagationdelay;
        } else {
            r_packet.total_delay += linkinfo[link].propagationdelay;
        }
        r_packet.hop_count = r_packet.hop_count + 1;

        // the current node is not the destination
        if (nodeinfo.address != r_packet.dest) {
            // if a route exist to this destination
            if (is_neighbour(r_packet.dest)) {
                int next_link = get_next_link_for_dest(r_packet.dest);
                uint16_t request_size = ROUTE_PACKET_SIZE(r_packet);
                DATAGRAM rreq_datagram = alloc_datagram(__ROUTING__, r_packet.source,
                                                   r_packet.dest, (char*) &r_packet,
                                                   request_size);
                insert_reverse_route(r_packet.source,r_packet.dest,link,r_packet.req_id);
                send_packet_to_link(next_link,rreq_datagram);
            } else {
                // if no route rebroadcast
                uint16_t request_size = ROUTE_PACKET_SIZE(r_packet);
                DATAGRAM rreq_datagram =
                        alloc_datagram(__ROUTING__, r_packet.source,
                                       r_packet.dest, (char*) &r_packet, request_size);
                insert_reverse_route(r_packet.source, r_packet.dest,link, r_packet.req_id);
                broadcast_packet(rreq_datagram,link);
            }
        } else {
            // learn the route table
            insert_reverse_route(r_packet.source, r_packet.dest, link, r_packet.req_id);

            // send RREP
            ROUTE_PACKET rrep_packet;
            rrep_packet.source = r_packet.source;
            rrep_packet.dest = r_packet.dest;
            rrep_packet.type = RREP;
            rrep_packet.req_id = r_packet.req_id;
            rrep_packet.forward_min_mtu = r_packet.min_mtu;
            rrep_packet.min_mtu = linkinfo[link].mtu;
            rrep_packet.total_delay = linkinfo[link].propagationdelay;
            rrep_packet.hop_count = 0;
            uint16_t request_size = ROUTE_PACKET_SIZE(rrep_packet);
            DATAGRAM rrep_datagram =
                    alloc_datagram(__ROUTING__, r_packet.dest, r_packet.source,
                                   (char*) &rrep_packet, request_size);

            send_packet_to_link(link,rrep_datagram);
        }
        break;
    case RREP:
        r_packet.hop_count = r_packet.hop_count + 1;

        if (nodeinfo.address != r_packet.source) {
            // insert into the forward table
            insert_forward_route(r_packet.source, r_packet.dest, link, r_packet.req_id);
            // find a reverse link
            int reverse_link = find_reverse_route(r_packet.source, r_packet.dest,
                                                  r_packet.req_id);
            if (reverse_link == -1) {
                abort();
            }
            if (r_packet.min_mtu > linkinfo[reverse_link].mtu) {
                r_packet.min_mtu = linkinfo[reverse_link].mtu;
            }
            r_packet.total_delay += linkinfo[reverse_link].propagationdelay;
            uint16_t request_size = ROUTE_PACKET_SIZE(r_packet);
            DATAGRAM rrep_datagram =
                    alloc_datagram(__ROUTING__, r_packet.dest, r_packet.source,
                                   (char*) &r_packet, request_size);
            send_packet_to_link(reverse_link,rrep_datagram);

        } else {
            learn_route_table(r_packet.dest, r_packet.hop_count, link, r_packet.min_mtu,
                              r_packet.total_delay, r_packet.req_id);
            pending_route_requests[r_packet.dest]=NULLTIMER;
            route_notified[r_packet.dest] += 1;
            if (route_notified[r_packet.dest] == 1) {
                signal_transport(MTU_DISCOVERED,(CnetData)r_packet.dest);
            }
        }
        break;
    }
}
//-----------------------------------------------------------------------------
// send a route request
void send_route_request(CnetAddr destaddr, int time_to_live) {
    if (pending_route_requests[destaddr] == NULLTIMER) {
        ROUTE_PACKET req_packet;
        req_packet.type = RREQ;
        req_packet.source = nodeinfo.address;
        req_packet.dest = destaddr;
        req_packet.hop_count = 0;
        req_packet.total_delay = 0;
        req_packet.req_id = route_req_id;
        req_packet.min_mtu = 0;
        //req_packet.time_to_live = time_to_live;
        route_req_id++;
        uint16_t request_size = ROUTE_PACKET_SIZE(req_packet);
        DATAGRAM r_packet = alloc_datagram(__ROUTING__, nodeinfo.address,
                                           destaddr, (char*) &req_packet,
                                           request_size);

        // start timer for pending route request
        pending_route_requests[destaddr] =
                CNET_start_timer(EV_ROUTE_PENDING_TIMER, ROUTE_REQUEST_TIMEOUT, destaddr);
        pending_route_request_ttl[destaddr] = time_to_live;
        route_notified[destaddr] = 0;

        // insert into local history table
        insert_local_history(req_packet.source,req_packet.dest, req_packet.req_id);
        broadcast_packet(r_packet, -1);
    }
}
//-----------------------------------------------------------------------------
// detect fragmentation size for the specified destination
int get_mtu(CnetAddr address, int need_send_mtu_request) {
    if (address!=nodeinfo.address) {
        if (route_exists(address) == 1) {
            int t = find_address(address);
            return route_table[t].min_mtu - PACKET_HEADER_SIZE - DATAGRAM_HEADER_SIZE - DL_FRAME_HEADER_SIZE;
        } else {
            if (need_send_mtu_request==TRUE)
                send_route_request(address,1);
            return -1;
        }
    } else
        return -1;
}
//-----------------------------------------------------------------------------
// resend a route request
void route_request_resend(CnetEvent ev, CnetTimerID timer, CnetData data) {
    int address = (int) data;
    if (route_exists(address) == 0) {
        pending_route_requests[address] = NULLTIMER;
        send_route_request(address,pending_route_request_ttl[address]+1);
    }
}
//-----------------------------------------------------------------------------
//  initialize the routing protocol
void init_routing() {
    route_table = (ROUTE_TABLE *) malloc(sizeof(ROUTE_TABLE));
    route_table_size = 0;
    history_table = (HISTORY_TABLE*) malloc(sizeof(HISTORY_TABLE));
    history_table_size = 0;
    reverse_route = (REVERSE_ROUTE_TABLE*)malloc(sizeof(REVERSE_ROUTE_TABLE));
    reverse_table_size = 0;
    forward_route = (FORWARD_ROUTE_TABLE*)malloc(sizeof(FORWARD_ROUTE_TABLE));
    forward_table_size = 0;
    route_req_id = 0;
    // init route requests
    for (int i = 0; i < MAX_HOSTS_NUMBER; i++) {
        pending_route_requests[i] = NULLTIMER;
        pending_route_request_ttl[i] = 0;
        route_notified[i]=-1;
        routes[i] = 0;
    }
    // register handler for resending route requests
    CHECK(CNET_set_handler(EV_ROUTE_PENDING_TIMER,route_request_resend, 0));
}
//-----------------------------------------------------------------------------
// get a propagation delay on the way to destaddr
int get_propagation_delay(CnetAddr destaddr) {
    if (route_exists(destaddr)) {
        int t = find_address(destaddr);
        return route_table[t].total_delay;
    } else
        return -1;
}
//-----------------------------------------------------------------------------
void shutdown_routing() {
    uint64_t routing_memory = 0;
    routing_memory += route_table_size*sizeof(ROUTE_TABLE);
    routing_memory += history_table_size*sizeof(HISTORY_TABLE);
    routing_memory += reverse_table_size*sizeof(REVERSE_ROUTE_TABLE);
    routing_memory += forward_table_size*sizeof(FORWARD_ROUTE_TABLE);

    fprintf(stderr, "\trouting memory: %f(MB)\n", (double)routing_memory / (double)(8*1024*1024));

    free(route_table);
    free(history_table);
    free(reverse_route);
    free(forward_route);
}
//-----------------------------------------------------------------------------
