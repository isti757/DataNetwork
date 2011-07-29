/*
 * datalink_layer.h
 *
 *  Created on: Jul 29, 2011
 *      Author: isti
 */

#ifndef DATALINK_LAYER_H_
#define DATALINK_LAYER_H_

#include "frame.h"

typedef struct datalink_layer {

	// initialize datalink layer
	void init_datalink()

	// write an incoming frame into datalink layer
	void read_to_datalink(int link, FRAME frame)

	// write an outcoming packet into datalink layer
	void write_datalink(int link, PACKET packet)

} datalink_layer;

#endif /* DATALINK_LAYER_H_ */
