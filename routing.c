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
//The reverse route table
REVERSE_ROUTE_TABLE *reverse_route;
int reverse_table_size;
//-----------------------------------------------------------------------------
//The forward route table
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
//an array of route notifications(if 1 then already notified the TL)
uint8_t route_notified[MAX_HOSTS_NUMBER];
//a bitmap to find out if a route is discovered
uint8_t routes[MAX_HOSTS_NUMBER];
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
    if (link<=0) {
        fprintf(stderr,"Whichlink: link=%d host %d\n",link,nodeinfo.address);
    }
    return link;
}
//-----------------------------------------------------------------------------
// learn routing table TODO what about the best route?
void learn_route_table(CnetAddr address, int hops, int link, int mtu, CnetTime total_delay,int req_id) {
    if (route_exists(address) == 0) {
        fprintf(routing_log,"Host %d learns about %d link %d\n",nodeinfo.address,address,link);
        int t = find_address(address);
        route_table[t].minhops = hops;
        route_table[t].minhop_link = link;
        route_table[t].min_mtu = mtu;
        route_table[t].total_delay = total_delay;
        route_table[t].req_id = req_id;
        routes[address] = 1;
        fprintf(table_log,"Record: addr:%d, link:%d, mtu:%d,delay:%lld req_id:%d\n",address,
                link,mtu,total_delay,req_id);
    }

}
//-----------------------------------------------------------------------------
//insert an item into the reverse route table
int insert_reverse_route(CnetAddr source,CnetAddr dest, int link, int req_id) {
    reverse_route = (REVERSE_ROUTE_TABLE *) realloc((char *) reverse_route,
                                                    (reverse_table_size + 1) * sizeof(REVERSE_ROUTE_TABLE));
    reverse_route[reverse_table_size].source = source;
    reverse_route[reverse_table_size].dest = dest;
    reverse_route[reverse_table_size].reverse_link = link;
    reverse_route[reverse_table_size].req_id = req_id;
    fprintf(reverse_log,"Reverse rec: src:%d, dest:%d,link %d, rid %d\n",
            source,dest,link,req_id);
    return (reverse_table_size++);
}
//-----------------------------------------------------------------------------
//insert an item into the forward route table
int insert_forward_route(CnetAddr source,CnetAddr dest, int link, int req_id) {
    forward_route = (FORWARD_ROUTE_TABLE*)realloc((char*)forward_route,
                                                  (forward_table_size+1) * (sizeof(FORWARD_ROUTE_TABLE)));
    forward_route[forward_table_size].source = source;
    forward_route[forward_table_size].dest = dest;
    forward_route[forward_table_size].forward_link = link;
    forward_route[forward_table_size].req_id = req_id;

    fprintf(forward_log,"Forward rec: src:%d, dest:%d,link:%d,rid:%d\n",
            source,dest,link,req_id);
    return (forward_table_size++);
}

//-----------------------------------------------------------------------------
//find a reverse route link
int find_reverse_route(CnetAddr source,CnetAddr dest,int req_id) {
    for (int t = 0; t < reverse_table_size; ++t)
         if (reverse_route[t].source == source && reverse_route[t].dest==dest
                 && reverse_route[t].req_id==req_id)
             return reverse_route[t].reverse_link;
    return -1;
}
//-----------------------------------------------------------------------------
//find a reverse route link
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
    if (route_exists(destaddr) == FALSE) {
        N_DEBUG("the address is not in route table\n");
        return -1;
    } else {
        N_DEBUG1("the address is in route table link=%d\n",
                 whichlink(destaddr));
        return whichlink(destaddr);
    }
}
//-----------------------------------------------------------------------------
//Find a request id of a discovered route
int get_request_id_for_dest(CnetAddr dest) {
    if (route_exists(dest)) {
        int t = find_address(dest);
        return route_table[t].req_id;
    } else {
        fprintf(stderr,"No route in get_request_id_for_dest\n");
        abort();
    }
}
//-----------------------------------------------------------------------------
//Find a request id of a reverse route
int get_request_id_reverse(CnetAddr source,CnetAddr dest) {
    for (int t = 0; t < reverse_table_size; ++t)
         if (reverse_route[t].source == source && reverse_route[t].dest==dest)
             return reverse_route[t].req_id;
    return -1;
}
//-----------------------------------------------------------------------------
//Find a request id of a reverse route by src and dest
int find_reverse_route_no_id(CnetAddr source,CnetAddr dest) {
    for (int t = 0; t < reverse_table_size; ++t)
         if (reverse_route[t].source == source && reverse_route[t].dest==dest)
             return reverse_route[t].reverse_link;
    return -1;
}
//-----------------------------------------------------------------------------
// take a packet from the transport layer and send it to the destination
void route(DATAGRAM dtg) {
    //N_DEBUG1("Perform routing to %d\n", dtg.dest);
    int link = -1;
    //use the route table, direct route
    if (dtg.src == nodeinfo.address) {
        link = get_next_link_for_dest(dtg.dest);
        //the route is discovered by this node
        if (link != -1) {
            dtg.req_id = get_request_id_for_dest(dtg.dest);
            fprintf(routing_log,"Found a route in get_next_link_for_dest dest %d link %d\n",dtg.dest,link);
        } else {
            //find a reverse route
            link = find_reverse_route_no_id(dtg.dest,dtg.src);
            dtg.req_id = get_request_id_reverse(dtg.dest,dtg.src);
            fprintf(routing_log,"Found a route in find_reverse_route_no_id dest %d link %d\n",dtg.dest,link);
        }
        fprintf(routing_log,"Datagram from %d to %d rid %d mtu:%d\n",dtg.src,dtg.dest,dtg.req_id,
                                                               dtg.declared_mtu);

    } else {
        //send to a reverse route or a forward route.The datagram from another src
        if (dtg.req_id<0) {
            fprintf(stderr,"Host %d, src %d, dest %d. rid=%d rid<0\n",nodeinfo.address,dtg.src,dtg.dest,dtg.req_id);
            abort();
        }
        link = find_forward_route(dtg.src,dtg.dest,dtg.req_id);
        if (link!=-1) {
            fprintf(routing_log,"Found a forward route src %d, dest %d, link %d\n",
                    dtg.src,dtg.dest,link);
        } else {
            link = find_reverse_route(dtg.dest,dtg.src,dtg.req_id);
            if (link != -1)
            fprintf(routing_log,"Found a reverse route src %d, dest %d, link %d\n",
                    dtg.src,dtg.dest,link);
        }
    }
    if (link < 1) {
        fprintf(stderr,"in route(): no address for %d host %d\n", dtg.dest,nodeinfo.address);
        fprintf(routing_log,"in route(): no address for %d host %d\n", dtg.dest,nodeinfo.address);
        exit(-1);
    } else {
        N_DEBUG2("in route(): send to link %d\n to address=%d\n", link, dtg.dest);
        send_packet_to_link(link, dtg);
    }
}
//-----------------------------------------------------------------------------
//find an item in the local history
int find_local_history(CnetAddr source,CnetAddr dest, int req_id) {
    for (int t = 0; t < history_table_size; ++t) {
        if ((history_table[t].source == source) && (history_table[t].dest==dest)
                && (history_table[t].req_id == req_id)) {
            return t;
        }
    }
    return -1;
}
//-----------------------------------------------------------------------------
// insert an item in the local history
int insert_local_history(CnetAddr source,CnetAddr dest, int req_id) {
    history_table = (HISTORY_TABLE *) realloc((char *) history_table,
                                              (history_table_size + 1) * sizeof(HISTORY_TABLE));
    history_table[history_table_size].source = source;
    history_table[history_table_size].req_id = req_id;
    history_table[history_table_size].dest = dest;
    return (history_table_size++);
}
//-----------------------------------------------------------------------------
//Check if a host is a neighbour
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
        fprintf(routing_log,"Host %d:received rreq for %d link=%d\n", nodeinfo.address, r_packet.dest,link);
        // check local history if we already processed this RREQ
        if (find_local_history(r_packet.source,r_packet.dest, r_packet.req_id) != -1) {
            fprintf(routing_log,"Host %d:ignored by local history src=%d dest=%d\n",nodeinfo.address,r_packet.source,r_packet.dest);
            break;
        }
        // insert into local history table
        insert_local_history(r_packet.source,r_packet.dest,r_packet.req_id);
        //set up mtu and delays
        if (r_packet.min_mtu==0) {
            r_packet.min_mtu = linkinfo[link].mtu;
        } else {
            if (r_packet.min_mtu > linkinfo[link].mtu) {
                r_packet.min_mtu = linkinfo[link].mtu;
            }
        }
        if (r_packet.total_delay==0) {
            r_packet.total_delay = linkinfo[link].propagationdelay;
        } else {
            r_packet.total_delay +=linkinfo[link].propagationdelay;
        }
        r_packet.hop_count = r_packet.hop_count + 1;

        //the current node is not the destination
        if (nodeinfo.address != r_packet.dest) {
            //if a route exist to this destination
            if (is_neighbour(r_packet.dest)) {
                int next_link = get_next_link_for_dest(r_packet.dest);
                uint16_t request_size = ROUTE_PACKET_SIZE(r_packet);
                DATAGRAM rreq_datagram = alloc_datagram(__ROUTING__, r_packet.source,
                                                   r_packet.dest, (char*) &r_packet,
                                                   request_size);
                fprintf(routing_log,"RREQ src %d dest %d: a route exists, send to link %d mtu:%d delay %lld\n",
                        r_packet.source,r_packet.dest,next_link,r_packet.min_mtu,r_packet.total_delay);
                fprintf(routing_log,"RREQ src %d dest %d: make reverse record src:%d link:%d id:%d\n",
                        r_packet.source,r_packet.dest,r_packet.source,link,r_packet.req_id);
                insert_reverse_route(r_packet.source,r_packet.dest,link,r_packet.req_id);
                send_packet_to_link(next_link,rreq_datagram);
            } else {
                //if no route rebroadcast
                uint16_t request_size = ROUTE_PACKET_SIZE(r_packet);
                DATAGRAM rreq_datagram = alloc_datagram(__ROUTING__, r_packet.source,
                                                   r_packet.dest, (char*) &r_packet,
                                                   request_size);
                insert_reverse_route(r_packet.source,r_packet.dest,link,r_packet.req_id);
                fprintf(routing_log,"RREQ src %d dest %d: no route exist, broadcast exclude %d mtu:%d delay %lld\n",
                        r_packet.source,r_packet.dest,link,r_packet.min_mtu,r_packet.total_delay);
                fprintf(routing_log,"RREQ src %d dest %d: make reverse record src:%d link:%d id:%d\n",
                        r_packet.source,r_packet.dest,r_packet.source,link,r_packet.req_id);
                broadcast_packet(rreq_datagram,link);
            }

        } else {
            fprintf(routing_log,"RREQ src %d dest %d: reached destination mtu:%d delay %lld\n",
                    r_packet.source,r_packet.dest,r_packet.min_mtu,r_packet.total_delay);
            //learn the route table
            insert_reverse_route(r_packet.source,r_packet.dest,link,r_packet.req_id);
            //learn_route_table(r_packet.source,r_packet.hop_count,link,r_packet.min_mtu,r_packet.total_delay,r_packet.req_id);
            //send RREP
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
            DATAGRAM rrep_datagram = alloc_datagram(__ROUTING__, r_packet.dest,
                                               r_packet.source, (char*) &rrep_packet,
                                               request_size);

            fprintf(routing_log,"RREQ src %d dest %d: sending RREP\n",r_packet.source,r_packet.dest);
            send_packet_to_link(link,rrep_datagram);
        }
        break;
    case RREP:
        fprintf(routing_log,"Received rrep on %d link=%d\n", nodeinfo.address, link);
        r_packet.hop_count = r_packet.hop_count + 1;

        if (nodeinfo.address != r_packet.source) {
            fprintf(routing_log,"RREP to src %d from %d mtu:%d delay:%lld\n",r_packet.source,r_packet.dest,
                    r_packet.min_mtu,r_packet.total_delay);
            //insert into the forward table
            insert_forward_route(r_packet.source,r_packet.dest,link,r_packet.req_id);
            //find a reverse link
            int reverse_link = find_reverse_route(r_packet.source,r_packet.dest,r_packet.req_id);
            if (reverse_link==-1) {
                fprintf(stderr,"Fatal, no reverse route on host %d, RREP src %d dest %d\n",
                        nodeinfo.address,r_packet.source,r_packet.dest);
                abort();
            }
            if (r_packet.min_mtu > linkinfo[reverse_link].mtu) {
                r_packet.min_mtu = linkinfo[reverse_link].mtu;
            }
            r_packet.total_delay += linkinfo[reverse_link].propagationdelay;
            uint16_t request_size = ROUTE_PACKET_SIZE(r_packet);
            DATAGRAM rrep_datagram = alloc_datagram(__ROUTING__, r_packet.dest,
                                               r_packet.source, (char*) &r_packet,
                                               request_size);
            send_packet_to_link(reverse_link,rrep_datagram);

        } else {
            //the RREP reached its source
            if (r_packet.min_mtu!=r_packet.forward_min_mtu) {
                fprintf(stderr,"MTU mismatch host %d src %d dest %d mtu %d mtu_forw %d\n",
                        nodeinfo.address,r_packet.source,r_packet.dest,
                        r_packet.min_mtu,r_packet.forward_min_mtu);
            }
            fprintf(routing_log,"RREP:reached src %d dest %d mtu %d forward mtu %d\n",
                    r_packet.source,r_packet.dest,r_packet.min_mtu,r_packet.forward_min_mtu);
            learn_route_table(r_packet.dest,r_packet.hop_count,link,r_packet.min_mtu,r_packet.total_delay,r_packet.req_id);
            pending_route_requests[r_packet.dest]=NULLTIMER;
            route_notified[r_packet.dest] += 1;
            if (route_notified[r_packet.dest] == 1) {
                fprintf(routing_log,"RREP:Signal transport about %d mtu:%d delay:%lld\n",r_packet.dest,
                        r_packet.min_mtu,r_packet.total_delay);
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

        fprintf(routing_log,"sending rreq for %d\n", destaddr);
        // start timer for pending route request
        pending_route_requests[destaddr] = CNET_start_timer(
                    EV_ROUTE_PENDING_TIMER, ROUTE_REQUEST_TIMEOUT, destaddr);
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
            fprintf(routing_log,"Host %d getting mtu for %d, result:%d\n",nodeinfo.address,address,route_table[t].min_mtu);
            return route_table[t].min_mtu - PACKET_HEADER_SIZE - DATAGRAM_HEADER_SIZE - DL_FRAME_HEADER_SIZE;
        } else {
            fprintf(routing_log,"Host %d getting mtu for %d, sending request\n",nodeinfo.address,address);
            if (need_send_mtu_request==TRUE) {
                send_route_request(address,1);
            }
            return -1;
        }
    } else {
        return -1;
    }
}
//-----------------------------------------------------------------------------
//Resend a route request
void route_request_resend(CnetEvent ev, CnetTimerID timer, CnetData data) {
    int address = (int) data;
    N_DEBUG1("Checking route request for %d\n", address);
    if (route_exists(address) == 0) {
        pending_route_requests[address] = NULLTIMER;
        N_DEBUG1("Resending route request for %d\n", address);
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


    char filename [] = "routing_log";
    sprintf(filename+8, "%3d", nodeinfo.address);
    routing_log = fopen(filename, "a");

    char filename1 [] = "tableroulog";;
    sprintf(filename1+8, "%3d", nodeinfo.address);
    table_log = fopen(filename1, "a");

    char filename2 [] = "reverse_log";;
    sprintf(filename2+8, "%3d", nodeinfo.address);
    reverse_log = fopen(filename2, "a");

    char filename3 [] = "forward_log";;
    sprintf(filename3+8, "%3d", nodeinfo.address);
    forward_log = fopen(filename3, "a");

}
//-----------------------------------------------------------------------------
//Get a propagation delay on the way to destaddr
int get_propagation_delay(CnetAddr destaddr) {
    if (route_exists(destaddr)) {
        int t = find_address(destaddr);
        return route_table[t].total_delay;
    } else {
        fprintf(stderr,"No route for %d\n",destaddr);
        return -1;
    }
}
//-----------------------------------------------------------------------------
void shutdown_routing() {
    free(route_table);
    free(history_table);
    free(reverse_route);
    free(forward_route);
}
//-----------------------------------------------------------------------------
