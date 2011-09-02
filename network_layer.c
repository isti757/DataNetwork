/*
 * network_layer.c
 *
 *  Created on: Jul 30, 2011
 *      Author: isti
 */

#include <cnet.h>

#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "debug.h"
#include "network_layer.h"

//-----------------------------------------------------------------------------
//initialize the network table
void init_network() {
	init_routing();
	init_discovery();
}
//-----------------------------------------------------------------------------
// read an incoming packet into network layer
void write_network(uint8_t kind, CnetAddr address,uint16_t length, char* packet) {
	N_DEBUG("Call write_network\n");

	DATAGRAM dtg;
	dtg.src = nodeinfo.address;
	dtg.dest = address;
    dtg.kind = kind | __TRANSPORT__;
	dtg.length = length;

	size_t packet_length = length;
	memcpy(&(dtg.payload), packet, packet_length);

	route(dtg);
	//N_DEBUG1("Size of the queue=%d\n", queue_nitems(datagram_queue));
}
//-----------------------------------------------------------------------------
// write an incoming message from datalink to network layer
void read_network(int link, size_t length, char * datagram) {
	DATAGRAM dtg;
	memcpy(&dtg, datagram, length);
	N_DEBUG1("Received checksum=%d\n", dtg.checksum);
	// TODO: fix checksums
//	uint16_t checksum = dtg.checksum;
//	dtg.checksum = 0;
//	size_t len = DATAGRAM_SIZE(dtg);
//	uint16_t checksum_to_compare = CNET_ccitt((unsigned char *) &dtg, len);
//	if (checksum_to_compare != checksum) {
//	    N_DEBUG("BAD checksum - ignored\n");
//		return; // bad checksum, ignore frame
//	}
	N_DEBUG1("Dispatching %d...\n",dtg.kind);

	//Dispatch the datagram
	if (is_kind(dtg.kind,__DISCOVER__))
		do_discovery(link, dtg);
	if (is_kind(dtg.kind,__ROUTING__))
		do_routing(link, dtg);
	if (is_kind(dtg.kind,__TRANSPORT__) ) {
		N_DEBUG("received datagram on transport level\n");
		if (dtg.dest != nodeinfo.address) {
			N_DEBUG("forwarding..\n");
			route(dtg);
		} else {
			read_transport(dtg.kind, dtg.length, dtg.src, (char*)dtg.payload);
		}
	}
}
//-----------------------------------------------------------------------------
// allocate a datagram
DATAGRAM alloc_datagram(uint8_t prot, int src, int dest, char *p, uint16_t len) {
	DATAGRAM np;
	np.kind = prot;
	np.src = src;
	np.dest = dest;
	//np->hopCount = MAX_HOP_COUNT;
	np.length = len;
	memcpy((char*)np.payload, (char*)p, len);
	return np;
}
//-----------------------------------------------------------------------------
// send a packet to address
void send_packet(CnetAddr addr, DATAGRAM datagram) {
	int link;
	// get link for node
	link = get_next_link_for_dest(addr);
	send_packet_to_link(link, datagram);
}
//-----------------------------------------------------------------------------
// send a packet to the link
void send_packet_to_link(int link, DATAGRAM datagram) {
	size_t size = DATAGRAM_SIZE(datagram);
	datagram.checksum = 0;
	datagram.checksum = CNET_ccitt((unsigned char *) &datagram, size);

	// send packet down to link layer
	write_datalink(link, (char *) &datagram, size);
}
//-----------------------------------------------------------------------------
// broadcast datagram to all links(excluded one)
void broadcast_packet(DATAGRAM dtg, int exclude_link) {
	for (int i = 1; i <= nodeinfo.nlinks; i++) {
		if (i != exclude_link) {
			send_packet_to_link(i, dtg);
		}
	}
}
//-----------------------------------------------------------------------------
void shutdown_network() {
    shutdown_routing();
    shutdown_datalink();
}
//-----------------------------------------------------------------------------
