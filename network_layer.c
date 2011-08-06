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
    datagram_queue = queue_new();
}
//-----------------END OF PRIVATE FUNCTIONS------------------------------------
//-----------------------------------------------------------------------------
// read an incoming packet into network layer
void write_network(CnetAddr destaddr, PACKET pkt) {
	DATAGRAM dtg;
	dtg.src = nodeinfo.address;
	dtg.dest = destaddr;
	memcpy(&(dtg.payload), &pkt,PACKET_SIZE(pkt));
	dtg.length = PACKET_SIZE(pkt);
	dtg.kind = TRANSPORT;
	dtg.timesent = nodeinfo.time_in_usec;
	queue_add(datagram_queue, &dtg, DATAGRAM_SIZE(dtg));
	printf("Size of the queue=%d\n",queue_nitems(datagram_queue));
}
//-----------------------------------------------------------------------------
// write an incoming message from datalink to network layer
void read_network(int link, DATAGRAM dtg, int length)
{
	switch (dtg.kind) {
		case DISCOVER:
			do_discovery(link,dtg);
			break;
		case ROUTING:
			do_routing(link,dtg);
			break;
		case TRANSPORT:
			printf("received datagram on transport level\n");
			if (dtg.dest!=nodeinfo.address) {
				printf("forwarding..\n");
				queue_add(datagram_queue, &dtg, DATAGRAM_SIZE(dtg));
			} else {
				read_transport(dtg);
			}
			break;
		default:
			break;
	}
}

/* Allocate a datagram
*/
DATAGRAM* alloc_datagram(int prot, int src, int dest, char *p, int len) {

  int plen;
  DATAGRAM* np;

   //allocate memory for network packet
  plen = sizeof(DATAGRAM) + len;

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
/* send a packet to link from Node Address */
void send_packet(CnetAddr addr, DATAGRAM datagram)
{

  int link;

  /* get link for node */
  link = get_next_link_for_dest(addr);

  send_packet_to_link(link, datagram);
}

//-----------------------------------------------------------------------------
/* send a packet to link */
void send_packet_to_link(int link, DATAGRAM datagram)
{

  int size;

  /* size of this packet */
  size = DATAGRAM_SIZE(datagram);

  /* send packet down to link layer */
  write_datalink(link, (char *)&datagram, size);
}
void broadcast_packet(DATAGRAM dtg,int exclude_link)
{
	int i = 0;
	for (i = 0; i < nodeinfo.nlinks; i++) {
		int link = i + 1; //link number
		if (link!=exclude_link) {
			send_packet_to_link(link,dtg);
		}
	}
}


