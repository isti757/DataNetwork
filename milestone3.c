/*
 * milestone1.c
 *
 *  Created on: Jul 18, 2011
 *      Author: isti
 */

#include <cnet.h>
#include <stdlib.h>
#include <string.h>

#include "datalink_layer.h"
#include "network_layer.h"
#include "transport_layer.h"
#include "application_layer.h"


//-----------------------------------------------------------------------------
EVENT_HANDLER(reboot_node) {
    //init the datalink layer
    init_datalink();
    //init the network layer
    init_network();
    //init the transport layer
    init_transport();
	//CNET_enable_application(ALLNODES);
}
