/*
 * application.c
 *
 *  Created on: 06.08.2011
 *      Author: kirill
 */
#include "application_layer.h"
CnetTimerID discovery_pending_timer;

//-----------------------------------------------------------------------------
//enables application after finishing neighbor discovery process
void discovery_pending(CnetEvent ev, CnetTimerID timer, CnetData data) {
	printf("checking for discovery to finish...\n");
	if (check_neighbors_discovered() == 0) {
		printf("Waiting more..\n");
		discovery_pending_timer = CNET_start_timer(EV_DISCOVERY_PENDING_TIMER,
				DISCOVERY_PENDING_TIME, 0);
	} else {
		//if (nodeinfo.address==182)
		CNET_enable_application(ALLNODES);
	}
}
//-----------------------------------------------------------------------------
//init application layer
void init_application() {
	CHECK(CNET_set_handler(EV_DISCOVERY_PENDING_TIMER, discovery_pending, 0));
	discovery_pending_timer = CNET_start_timer(EV_DISCOVERY_PENDING_TIMER,
			DISCOVERY_PENDING_TIME, 0);
}
