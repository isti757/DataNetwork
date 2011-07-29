/*
 * application_layer.h
 *
 *  Created on: Jul 29, 2011
 *      Author: isti
 */

#ifndef APPLICATION_LAYER_H_
#define APPLICATION_LAYER_H_

#include "frame.h"

typedef struct application_layer {

	// initialize the layer internal structures
	void init_application();

	// reads incoming message from transport to application layer
	void read_to_application(MSG message);

	// write the message up to the application
	void write_from_application(CnetAddr addr, MSG msg);

} application_layer;

#endif /* APPLICATION_LAYER_H_ */
