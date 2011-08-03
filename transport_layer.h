/*
 * transport_layer.h
 *
 *  Created on: Jul 29, 2011
 *      Author: isti
 */

#ifndef TRANSPORT_LAYER_H_
#define TRANSPORT_LAYER_H_

#include <string.h>
#include "packet.h"
#include "network_layer.h"
#include "application_layer.h"
//-----------------------------------------------------------------------------
//typedef struct transport_layer {
//
//};
//-----------------------------------------------------------------------------
// initialize transport layer
extern void init_transport();
//-----------------------------------------------------------------------------
// read outgoing message from application to transport layer
extern void write_transport(CnetEvent ev, CnetTimerID timer, CnetData data);
//-----------------------------------------------------------------------------
// write outcoming message from network to transport
extern void read_transport(DATAGRAM datagram);
//-----------------------------------------------------------------------------

#endif /* TRANSPORT_LAYER_H_ */
