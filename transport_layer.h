/*
 * transport_layer.h
 *
 *  Created on: Jul 29, 2011
 *      Author: isti
 */

#ifndef TRANSPORT_LAYER_H_
#define TRANSPORT_LAYER_H_

#include "frame.h"

typedef struct transport_layer {

	// initialize transport layer
	void init_transport();

	// read incoming message from network to transport layer
	void read_to_transport(PACKET pkt);

	// write outcoming message from application into transport layer
	void write_from_transport(CnetAddr addr, MSG msg);
};

#endif /* TRANSPORT_LAYER_H_ */
