/*
 * application_layer.h
 *
 *  Created on: Jul 29, 2011
 *      Author: isti
 */

#ifndef APPLICATION_LAYER_H_
#define APPLICATION_LAYER_H_

#include "frame.h"

#include "transport_layer.h"

//-----------------------------------------------------------------------------
typedef struct application_layer_t {


} application_layer_t;
//-----------------------------------------------------------------------------
application_layer_t application_layer;
//-----------------------------------------------------------------------------
static EVENT_HANDLER(application_ready) {
	// read the message from application layer
	CnetAddr destaddr;

	MSG msg;
	int msg_size = sizeof(MSG);
	CHECK(CNET_read_application(&destaddr, (char *)msg, &msg_size));

	// pass message to transport
	read_application(addr, msg, msg_size);
}
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------

#endif /* APPLICATION_LAYER_H_ */
