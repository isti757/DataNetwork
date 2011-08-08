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
//timer setting for pending discovery process
#define DISCOVERY_PENDING_TIME 20000
#define EV_DISCOVERY_PENDING_TIMER  EV_TIMER8
//-----------------------------------------------------------------------------
// initialize transport layer
extern void init_transport();
//-----------------------------------------------------------------------------
// read outgoing message from application to transport layer
extern void write_transport(CnetEvent ev, CnetTimerID timer, CnetData data);
//-----------------------------------------------------------------------------
// write incoming message from network to transport
extern void read_transport(uint16_t,char*);
//-----------------------------------------------------------------------------

#endif /* TRANSPORT_LAYER_H_ */
