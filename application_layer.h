/*
 * application_layer.h
 *
 *  Created on: Jul 29, 2011
 *      Author: isti
 */

#ifndef APPLICATION_LAYER_H_
#define APPLICATION_LAYER_H_

#include "cnet.h"
#include "packet.h"
#include "transport_layer.h"
#include "routing.h"
//timer setting for pending discovery process
#define DISCOVERY_PENDING_TIME 200000
#define EV_DISCOVERY_PENDING_TIMER  EV_TIMER8

//-----------------------------------------------------------------------------
//init application
extern void init_application();
#endif /* APPLICATION_LAYER_H_ */
