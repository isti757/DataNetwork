/*
 * routing.h
 *
 *  Created on: 02.08.2011
 *      Author: kirill
 */

#ifndef ROUTING_H_
#define ROUTING_H_
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

//-----------------------------------------------------------------------------
// initialize the routing
extern void initRouting();
//-----------------------------------------------------------------------------
// route a packet
extern void route(CnetAddr, DATAGRAM);
//-----------------------------------------------------------------------------
// learn the routing table
extern void learn_route_table(CnetAddr address, int hops, int link, CnetTime usec);

//-----------------------------------------------------------------------------
extern void show_table(CnetEvent ev, CnetTimerID timer, CnetData data);

extern int up_to_network(DATAGRAM datagram, int length, int arrived_on);
#endif /* ROUTING_H_ */
