/*
 * discovery.c
 *
 *  Created on: 02.08.2011
 *      Author: kirill
 */
#include "cnet.h"
#include <stdio.h>
#include "discovery.h"
#include <malloc.h>
#include <string.h>
#include "datagram.h"

//-----------------------------------------------------------------------------
/* poll our neighbors */
void pollNeighbors();
void timerHandler(CnetEvent, CnetTimerID, CnetData);

/* poll response counter */
//static int pollCount;
/*
 Poll response timer number assigned by cnet */
 static int pollResponseTimer;

// Poll period timer number assigned by cnet
static int pollTimer;

int response_status[MAX_LINKS_COUNT];

/* initialize discovery protocol */
//-----------------------------------------------------------------------------
void init_discovery() {
	/* establish the timer event handler */
	CHECK(CNET_set_handler(EV_DISCOVERY_TIMER, timerHandler, 0));

	 //initialize the response status
	 for (int i = 0; i < nodeinfo.nlinks; i++) {
		 response_status[i+1] = 0;
	 }
	 pollTimer = CNET_start_timer(EV_DISCOVERY_TIMER, DiscoveryStartUp,POLLTIMER);

}
//-----------------------------------------------------------------------------
/* process a discovery protocol packet */
void do_discovery(int link, DATAGRAM datagram) {

	//printf("Doing discovery\n");
	DATAGRAM* np;
	DiscoveryPacket p;
	DiscoveryPacket dpkt;
	memcpy(&p,&datagram.payload,datagram.length);
	//process request/response
	switch (p.type) {
	//request that we identify ourselves - send I_Am reply
	case Who_R_U:
		dpkt.type = I_Am;
		dpkt.address = nodeinfo.address; //my address
		dpkt.timestamp = p.timestamp; //return sender's time stamp
		//send the packet
		np = alloc_datagram(DISCOVER, nodeinfo.address, p.address,
				(char *) &dpkt, sizeof(dpkt));
		send_packet_to_link(link, *np);
		break;

		//response to our Who_R_U query
	case I_Am:
		//if we are not "owed" any I-Am responses, ignore.
		printf("learning table...\n");
		learn_route_table(p.address,0,link,linkinfo[link].mtu,linkinfo[link].propagationdelay);
		printf("Poll response from %d %d\n",link,nodeinfo.time_in_usec);
		response_status[link] = 1;
		break;
	}
}
//-----------------------------------------------------------------------------
/* poll our neighbor */
void pollNeighbor(int link) {
	DiscoveryPacket p;
	int request = Who_R_U;
	DATAGRAM* np;
	/* send a Poll message*/
	p.type = request; //the request
	p.address = nodeinfo.address; //my address
	p.timestamp = nodeinfo.time_in_usec; //time we send query
	np = alloc_datagram(DISCOVER, nodeinfo.address, -1, (char *) &p,
				sizeof(p));
	send_packet_to_link(link, *np);

}
//-----------------------------------------------------------------------------
/* Discovery timer event handler */
void timerHandler(CnetEvent ev, CnetTimerID timer, CnetData data) {
	switch ((int) data) {

	/* time to poll neighbors again */
	case POLLTIMER:
		printf("Start polling..\n");
		for (int i=0;i<nodeinfo.nlinks;i++) {
			int link = i+1;
			pollNeighbor(link);
		}
		/* start poll fail timer for this poll period */
		pollResponseTimer = CNET_start_timer(EV_DISCOVERY_TIMER, DiscoveryTimeOut,POLLRESPONSETIMER);
		break;
	case POLLRESPONSETIMER:
		printf("Checking for polling responses\n");
		int run_timer = 0;
		for (int i=0;i<nodeinfo.nlinks;i++) {
			if (response_status[i+1]==0) {
				pollNeighbor(i+1);
				run_timer = 1;
			}
		}
		if (run_timer==1) {
			pollResponseTimer = CNET_start_timer(EV_DISCOVERY_TIMER, DiscoveryTimeOut,POLLRESPONSETIMER);
			printf("Timer started again\n");
		}
		break;
	}
}

