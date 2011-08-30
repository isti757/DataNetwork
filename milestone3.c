/*
 * milestone1.c
 *
 *  Created on: Jul 18, 2011
 *      Author: isti
 */

#include <cnet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "datalink_layer.h"
#include "network_layer.h"
#include "transport_layer.h"
#include "routing.h"
#include "discovery.h"



#include "datalink_layer.c"
#include "network_layer.c"
#include "transport_layer.c"
#include "routing.c"
#include "discovery.c"


//-----------------------------------------------------------------------------
EVENT_HANDLER(reboot_node) {

	//init the datalink layer
    init_datalink();
    //init the network layer
    init_network();
    //init the transport layer
    init_transport();
}
