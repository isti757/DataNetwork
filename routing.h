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

#define	MAXHOPS		4
#define	ALL_LINKS	(-1)
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

int route_req_id;

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
extern void route(CnetAddr, DATAGRAM);
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
extern void show_table(CnetEvent ev, CnetTimerID timer, CnetData data);

extern int up_to_network(DATAGRAM datagram, int length, int arrived_on);
#endif /* ROUTING_H_ */
