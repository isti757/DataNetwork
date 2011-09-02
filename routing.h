/*
 * routing.h
 *
 *  Created on: 02.08.2011
 *      Author: kirill
 */
#include <cnet.h>
#include <stdlib.h>

#ifndef ROUTING_H_
#define ROUTING_H_

//-----------------------------------------------------------------------------
#define EV_ROUTE_PENDING_TIMER  EV_TIMER7
// timeout for waiting route response
#define ROUTE_REQUEST_TIMEOUT 1000000
// the maximum number of hosts
#define MAX_HOSTS_NUMBER 256
//-----------------------------------------------------------------------------
// types of route requests
#define RREQ   1
#define RREP   2
//-----------------------------------------------------------------------------
// definition of a network routing table for each node
typedef struct {
    CnetAddr	address;
    long	minhops;
    long	minhop_link;
    int 	min_mtu;
    CnetTime	total_delay;
} ROUTE_TABLE;
//-----------------------------------------------------------------------------
// definition of a local history table for routing
typedef struct {
	CnetAddr source;
	int req_id;
} HISTORY_TABLE;
//-----------------------------------------------------------------------------
//A route request packet //TODO размер?
typedef struct {
	uint8_t type;
	uint8_t source;
	uint8_t dest;
	uint16_t hop_count;//TODO is it correct size?
	uint16_t req_id; //a local counter maintained separately by each node and incremented each time a ROUTE REQUEST is broadcast
	uint16_t min_mtu;//a minimum MTU value on the way to destination
	CnetTime total_delay;//a total propagation delay on the way to destination
} ROUTE_PACKET;
//-----------------------------------------------------------------------------
// initialize the routing
extern void init_routing();
//-----------------------------------------------------------------------------
// route a packet
extern void route(DATAGRAM);
//-----------------------------------------------------------------------------
// check if a route for specified address exists in routing table
extern int route_exists(CnetAddr address);
//-----------------------------------------------------------------------------
//send a route request to find address
extern void send_route_request(CnetAddr address);
//-----------------------------------------------------------------------------
//process a routing packet
extern void do_routing(int link,DATAGRAM datagram);
//-----------------------------------------------------------------------------
// learn the routing table
extern void learn_route_table(CnetAddr address, int hops, int link,int mtu,CnetTime total_delay);
//-----------------------------------------------------------------------------
// detect a link for outcoming message
extern int get_next_link_for_dest(CnetAddr destaddr);
//-----------------------------------------------------------------------------
// detect fragmentation size for the specified address
extern int get_mtu(CnetAddr);
//-----------------------------------------------------------------------------
//get propagation delay for specified address
extern int get_propagation_delay(CnetAddr);
//-----------------------------------------------------------------------------
extern void show_table(CnetEvent ev, CnetTimerID timer, CnetData data);
//-----------------------------------------------------------------------------
extern void shutdown_routing();
//-----------------------------------------------------------------------------

#endif /* ROUTING_H_ */
