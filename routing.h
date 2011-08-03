/*
 * routing.h
 *
 *  Created on: 02.08.2011
 *      Author: kirill
 */

#ifndef ROUTING_H_
#define ROUTING_H_

//-----------------------------------------------------------------------------
// initialize the routing
extern void initRouting();
//-----------------------------------------------------------------------------
// route a packet
extern void route(CnetAddr, DATAGRAM);

//-----------------------------------------------------------------------------
extern void show_table(CnetEvent ev, CnetTimerID timer, CnetData data);

extern int up_to_network(DATAGRAM datagram, int length, int arrived_on);
#endif /* ROUTING_H_ */
