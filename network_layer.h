/*
 * network_layer.h
 *
 *  Created on: Jun 28, 2011
 *      Author: isti
 */

#ifndef NETWORK_LAYER_H_
#define NETWORK_LAYER_H_

#include "frame.h"

typedef struct network_layer {

	// initialize network layer
	void init_network();

	// detect a link for outcoming message
	int get_next_link_for_dest(CnetAddr destaddr);

	// detect fragmentation size for the specified link
	int get_mtu_for_link(int link);

	// read an incoming packet into network layer
	void read_to_network(link, PACKET packet);

	// write an outgoing message from transport to network layer
	void write_from_network(int link, PACKET packet);
} network_layer;

#endif /* NETWORK_LAYER_H_ */
