/*
 * transport_layer.h
 *
 *  Created on: Jul 29, 2011
 *      Author: isti
 */

#ifndef TRANSPORT_LAYER_H_
#define TRANSPORT_LAYER_H_

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
// read incoming message from network to transport layer
extern void read_transport(CnetAddr destaddr, PACKET pkt);
//-----------------------------------------------------------------------------
// write outcoming message from application into transport layer
extern void write_transport(PACKET pkt, CnetAddr dest);
//-----------------------------------------------------------------------------

#endif /* TRANSPORT_LAYER_H_ */
