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
	initRouting();
	initDiscovery();
}
//-----------------END OF PRIVATE FUNCTIONS------------------------------------
//-----------------------------------------------------------------------------
// read an incoming packet into network layer
void write_network(CnetAddr destaddr, PACKET pkt) {
	DATAGRAM dtg;
	dtg.dest = destaddr;
	memcpy(&dtg.payload, &pkt,PACKET_SIZE(pkt));
	dtg.length = PACKET_SIZE(pkt);
	dtg.kind = TRANSPORT;



	route(dtg.dest,dtg);
	//selective_flood((char *) &dtg, DATAGRAM_SIZE(dtg), whichlink(dtg.dest));

	//read_datalink(link, dtg);
}
//-----------------------------------------------------------------------------
// write an incoming message from datalink to network layer
void read_network(int link, DATAGRAM dtg, int length)
{
	switch (dtg.kind) {
		case DISCOVER:
			doDiscovery(link,dtg);
			break;
		case ROUTING:

			break;
		case TRANSPORT:

			break;
		default:
			break;
	}


	// if not this host, send further
	// update routing tables
	// send it further
	CHECK(up_to_network(dtg, length, link));
}
//-----------------------------------------------------------------------------
// detect a link for outcoming message
int get_next_link_for_dest(CnetAddr destaddr)
{
	return 1;
}
//-----------------------------------------------------------------------------
// detect fragmentation size for the specified link
int get_mtu_for_link(int link)
{
	return 96;
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
