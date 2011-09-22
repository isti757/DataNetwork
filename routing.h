/*
 * transport_layer.h
 *
 *  Created on: Aug 30, 2011
 *      Author: kirill
 */
#include <stdlib.h>

#include <cnet.h>

#ifndef ROUTING_H_
#define ROUTING_H_

//-----------------------------------------------------------------------------
#define EV_ROUTE_PENDING_TIMER  EV_TIMER7
// timeout for waiting route response
#define ROUTE_REQUEST_TIMEOUT 2000000
// the maximum number of hosts
#define MAX_HOSTS_NUMBER 256
//-----------------------------------------------------------------------------
// types of route requests
#define RREQ   1
#define RREP   2
//-----------------------------------------------------------------------------
// definition of a network routing table for each node
typedef struct {
    uint8_t	address;
    int	        minhops;
    int	        minhop_link;
    int 	min_mtu;
    CnetTime	total_delay;
    uint16_t         req_id;
} __attribute__((packed)) ROUTE_TABLE;
//-----------------------------------------------------------------------------
// definition of a local history table for routing
typedef struct {
        uint8_t source;
        uint8_t dest;
        uint16_t req_id;
} __attribute__((packed)) HISTORY_TABLE;
//-----------------------------------------------------------------------------
// a reverse route table
typedef struct {
        uint8_t source;
        uint8_t dest;
        uint8_t reverse_link;
        uint16_t req_id;
} REVERSE_ROUTE_TABLE;
//-----------------------------------------------------------------------------
// a forward route table
typedef struct {
    uint8_t source;
    uint8_t dest;
    uint8_t forward_link;
    uint16_t req_id;
} FORWARD_ROUTE_TABLE;
//-----------------------------------------------------------------------------
// a route request packet
typedef struct {
	uint8_t type;
	uint8_t source;
	uint8_t dest;
        uint16_t hop_count;
        uint16_t req_id;           // a local counter maintained separately by each node
                              // and incremented each time a ROUTE REQUEST is sent
        uint16_t min_mtu;     // a minimum MTU value on the way to destination
        uint16_t forward_min_mtu;
        CnetTime total_delay; // a total propagation delay on the way to destination
} __attribute__((packed)) ROUTE_PACKET;

#define ROUTE_PACKET_SIZE(pkt) (4*sizeof(uint16_t)+3*sizeof(uint8_t)+sizeof(CnetTime))
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
// send a route request to find address
extern void send_route_request(CnetAddr address);
//-----------------------------------------------------------------------------
// process a routing packet
extern void do_routing(int link,DATAGRAM datagram);
//-----------------------------------------------------------------------------
// learn the routing table
extern void learn_route_table(CnetAddr address, int hops, int link, int mtu,
                              CnetTime total_delay, uint16_t req_id);
//-----------------------------------------------------------------------------
// detect a link for outcoming message
extern int get_next_link_for_dest(CnetAddr destaddr);
//-----------------------------------------------------------------------------
// detect a fragmentation size for the specified address
extern int get_mtu(CnetAddr dest, int need_send_mtu_request);
//-----------------------------------------------------------------------------
// get propagation delay on the route to specified address
extern int get_propagation_delay(CnetAddr);
//-----------------------------------------------------------------------------
extern void shutdown_routing();
//-----------------------------------------------------------------------------

#endif /* ROUTING_H_ */
