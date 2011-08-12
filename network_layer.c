/*
 * network_layer.c
 *
 *  Created on: Jul 30, 2011
 *      Author: isti
 */
#include <cnet.h>
#include <stdlib.h>
#include "network_layer.h"
#include "discovery.h"
#include "routing.h"

//initialize the network table
void init_network() {
	init_routing();
	init_discovery();
	//create an output queue for sending
	//datagram_queue = queue_new();
}
//-----------------------------------------------------------------------------
static int is_kind(uint16_t kind, PACKETKIND knd) {
    return ((kind & knd) > 0);
}
//-----------------------------------------------------------------------------
// read an incoming packet into network layer
void write_network(PACKETKIND kind, CnetAddr address,uint16_t length, char* packet) {
	printf("Call write_network\n");
	DATAGRAM dtg;
	dtg.src = nodeinfo.address;
	dtg.dest = address;
	size_t datagram_length = length;
	memcpy(&(dtg.payload), packet, datagram_length);
	dtg.length = length;
	dtg.kind = kind;
	dtg.timesent = nodeinfo.time_in_usec;
	route(dtg);
	//queue_add(datagram_queue, &dtg, DATAGRAM_SIZE(dtg));
	//printf("Size of the queue=%d\n", queue_nitems(datagram_queue));
}
//-----------------------------------------------------------------------------
// write an incoming message from datalink to network layer
void read_network(int link, DATAGRAM dtg) {
	if (is_kind(dtg.kind,DISCOVER))
		do_discovery(link, dtg);
	if (is_kind(dtg.kind,ROUTING))
		do_routing(link, dtg);
	if (is_kind(dtg.kind,TRANSPORT)) {
		printf("received datagram on transport level\n");
		if (dtg.dest != nodeinfo.address) {
			printf("forwarding..\n");
			route(dtg);
		} else {
			read_transport(dtg.kind,dtg.length,dtg.src,(char*)dtg.payload);
		}
	}
}
//-----------------------------------------------------------------------------
/* Allocate a datagram*/
DATAGRAM* alloc_datagram(int prot, int src, int dest, char *p, uint16_t len) {

	DATAGRAM* np;
	//allocate memory for network packet
	size_t plen = sizeof(DATAGRAM) + len;
	np = (DATAGRAM *) malloc(plen);
	np->kind = prot;
	np->src = src;
	np->dest = dest;
	//np->hopCount = MAX_HOP_COUNT;
	np->length = len;
	memcpy(np->payload, p, len);
	return np;
}
//-----------------------------------------------------------------------------
/* send a packet to address */
void send_packet(CnetAddr addr, DATAGRAM datagram) {
	int link;
	/* get link for node */
	link = get_next_link_for_dest(addr);
	send_packet_to_link(link, datagram);
}

//-----------------------------------------------------------------------------
/* send a packet to the link */
void send_packet_to_link(int link, DATAGRAM datagram) {
	int size;
	/* size of this packet */
	size = DATAGRAM_SIZE(datagram);
	datagram.checksum = 0;
	datagram.checksum = CNET_ccitt((unsigned char *) &datagram, size);
	/* send packet down to link layer */
	write_datalink(link, (char *) &datagram, size);
}
//-----------------------------------------------------------------------------
//broadcast datagram to all links(excluded one)
void broadcast_packet(DATAGRAM dtg, int exclude_link) {
	int i = 0;
	for (i = 0; i < nodeinfo.nlinks; i++) {
		int link = i + 1; //link number
		if (link != exclude_link) {
			send_packet_to_link(link, dtg);
		}
	}
}

