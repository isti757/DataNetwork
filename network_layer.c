/*
 * network_layer.c
 *
 *  Created on: Jul 30, 2011
 *      Author: isti
 */

#include "network_layer.h"

//-----------------------------------------------------------------------------
// read an incoming packet into network layer
void read_network(int link, DATAGRAM dtg)
{
	read_datalink(link, dtg);
}
//-----------------------------------------------------------------------------
// write an incoming message from datalink to network layer
void write_network(int link, DATAGRAM dtg)
{
	// if not this host, send further

	// update routing tables

	// send it further

	write_transport(dtg.data, dtg.src);
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
