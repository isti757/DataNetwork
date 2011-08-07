/*
 * routing.h
 *
 *  Created on: 02.08.2011
 *      Author: kirill
 */
#include <cnet.h>
#include <stdlib.h>
#include "datalink_layer.h"

#ifndef ROUTING_H_
#define ROUTING_H_

#define EV_ROUTE_PENDING_TIMER  EV_TIMER7
//timout for waiting route discovering
#define ROUTE_POLLING_TIME 200000

//A queue for outcoming datagrams
QUEUE datagram_queue;
/**
 * A network routing table for each node
 */
typedef struct {
    CnetAddr	address;
    int		ackexpected;
    int		nextpackettosend;
    int		packetexpected;
    long	minhops;
    long	minhop_link;
    CnetTime	totaltime;
} ROUTE_TABLE;

/*
 * A Local history table for routing
 */
typedef struct {
	CnetAddr source;
	int req_id;
} HISTORY_TABLE;

/*
 * A reverse route table
 */
typedef struct {
	CnetAddr source;
	int link;
} REVERSE_ROUTE_TABLE;

//The routing table
ROUTE_TABLE	*route_table;
int route_table_size;
//The history table
HISTORY_TABLE *history_table;
int history_table_size;
//The reverse route table
REVERSE_ROUTE_TABLE *reverse_route;
int reverse_table_size;
//route request counter
int route_req_id;
//types of route requests
#define RREQ   1
#define RREP   2
//A route request packet
typedef struct {
	int type;
	CnetAddr source;
	CnetAddr dest;
	int hop_count;
	int req_id; //a local counter maintained separately by each node and incremented each time a ROUTE REQUEST is broadcast
} ROUTE_PACKET;

//-----------------------------------------------------------------------------
// initialize the routing
extern void init_routing();
//-----------------------------------------------------------------------------
// route a packet
extern void route(CnetEvent ev, CnetTimerID timer, CnetData data);
//-----------------------------------------------------------------------------
//process a routing packet
extern void do_routing(int,DATAGRAM);
//-----------------------------------------------------------------------------
// learn the routing table
extern void learn_route_table(CnetAddr address, int hops, int link, CnetTime usec);
//-----------------------------------------------------------------------------
// detect a link for outcoming message
extern int get_next_link_for_dest(CnetAddr destaddr);
//-----------------------------------------------------------------------------
// detect fragmentation size for the specified link
extern int get_mtu_for_link(int link);
//-----------------------------------------------------------------------------
//check if all neighbors were discovered
extern int check_neighbors_discovered();
//-----------------------------------------------------------------------------
//send a route request to find address
extern void send_route_request(CnetAddr);
//-----------------------------------------------------------------------------

extern void show_table(CnetEvent ev, CnetTimerID timer, CnetData data);
#endif /* ROUTING_H_ */
