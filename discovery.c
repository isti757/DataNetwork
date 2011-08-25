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


void pollNeighbors();
void timerHandler (CnetEvent, CnetTimerID, CnetData);

/* poll response counter */
/*static int pollCount;

 Poll response timer number assigned by cnet
static int pollResponseTimer;

 Poll period timer number assigned by cnet
static int pollTimer;*/


/* initialize discovery protocol */

void initDiscovery() {
  /* establish the timer event handler */
  CHECK(CNET_set_handler(EV_DISCOVERY_TIMER, timerHandler, 0));

  /* establish the handler for EV_LINKSTATE
  CHECK(CNET_set_handler(EV_LINKSTATE, linkEvent, 0));

   initialize the linkstate table
  initLinkState();

   initialize the time of last response for each neighbor
  for (i = 0; i < MAXLINKS; timeLastResponse[i++] = int64_ZERO);

   Schedule the first discovery poll
  pollTimer = CNET_start_timer(EV_DISCOVERY_TIMER, DiscoveryStartUp, POLLTIMER);*/
}

/* process a discovery protocol packet */
void doDiscovery(int link, DATAGRAM datagram) {

  /*pNetPacket np;
  pDiscoveryPacket p;
  DiscoveryPacket dpkt;
  int i, rtt, lastRtt, weight;
  CnetInt64 delta64;
  extern pLinkStateTable linkstate;

  p  = (pDiscoveryPacket) packet->payload;


  i = link - 1;

   process request/response
  switch (p->type) {

     request that we identify ourselves - send I_Am reply
    case Who_R_U:

      dpkt.type = I_Am;
      dpkt.address = nodeinfo.address;  my address
      dpkt.timestamp = p->timestamp;    return sender's time stamp
       send the packet
      np = allocNetPacket(Discovery, nodeinfo.address, p->address,
                         (char *) &dpkt,  sizeof(dpkt));
      sendPacket(p->address, np);
      freeNetPacket(np);

      break;


     response to our Who_R_U  query or Advertisement
    case I_Am:


         if we are not "owed" any I-Am responses, ignore.


      if (pollCount == 0)
        break;

       Make sure address is valid (!= 0)
      if (p->address == 0) {
        fprintf(stderr,
                "%f  diDiscovery():address = 0 in I_Am packet from link %d\n",
                timeOfDay(), link);
      }

       compute rtt for this poll
      int64_SUB(delta64, nodeinfo.time_in_usec, p->timestamp);
      int64_L2I(rtt, delta64);

       save time of this response
      timeLastResponse[i] = nodeinfo.time_in_usec;

       see if same address as before
      if (linkstate->states[i].address != p->address) {
        linkstate->states[i].linkno = link;
        linkstate->states[i].address = p->address;
        linkstate->states[i].rtt = rtt/1000;

        fprintf(stdout, "Node %d: Link %d to %d up at %f:  RTT = %d\n",
	        nodeinfo.address, link, linkstate->states[i].address,
                timeOfDay(), linkstate->states[i].rtt);
      }

       last recorded RTT (0 if link is down)
      lastRtt = linkstate->states[i].rtt;

       average new rtt into running average
      if (lastRtt == 0)
        weight = 0;

      linkstate->states[i].rtt = (weight*lastRtt + rtt/1000)/(weight+1);

       if link was down and is now up, say so
      if (lastRtt == 0) {
         fprintf(stdout, "Node %d: Link %d to %d up at %f:  RTT = %d\n",
	         nodeinfo.address, link, linkstate->states[i].address,
                 timeOfDay(), linkstate->states[i].rtt);
      }

       see if all responses now received.  If so, stop timer
      if (pollCount > 0) {  if timer is still running
         if (--pollCount == 0)   if all responses received
           CNET_stop_timer(pollResponseTimer);   then stop timer
      }
      break;


     This packet is an Advertisement - timestamp ignored.
	   Instead, initialize the rtt to propagation delay for link

    case Advertise:

       see if this address is new

      if ( getLinkByAddr(p->address) == -1 ||
           linkstate->states[i].rtt == 0 ) {

         new neighbor! Add to linkstate and alert routing!

        linkstate->states[i].linkno = link;
        linkstate->states[i].address = p->address;
        int64_L2I(rtt, linkinfo[link].propagationdelay);
        linkstate->states[i].rtt = rtt/1000;   prop delay in msec
		linkstate->states[i].status = 1;
      }

      takeLinkStateChange(p->address, 1);

      break;

     we don't know what this type code means!
    default:
      fprintf(stderr,
              "%f  doDiscovery(): invalid packet type (%d) received from link %d\n",
              timeOfDay(), p->type, link);
      break;

  }  end switch */
}

/* Discovery timer event handler */
void timerHandler (CnetEvent ev, CnetTimerID timer, CnetData data) {

  /*CnetInt64 delta64;
  int delta;
  int i, link;*/

  switch ((int)data) {

    /* time to poll neighbors again */
    case POLLTIMER:
       pollNeighbors();
       break;

    /* at least one polled neighbor did not respond - find which */
    case POLLRESPONSETIMER:
      /* set pollCount to zero - we will find all dead links here */
      /*pollCount = 0;

         For each neighbor, check time of last poll response.
         If response is PollLimit or more DiscoveryTimeOut old,
         then declare link down

      for (i = 0; i < linkstate->nlinks; i++) {
         link = i + 1;
         int64_SUB(delta64, nodeinfo.time_in_usec, timeLastResponse[i]);
         int64_L2I(delta, delta64);
         if (delta >= PollLimit) {       link is down
           if (int64_IS_ZERO(timeLastResponse[i]) ||
               linkstate->states[i].rtt != 0) {  if not previously down
             fprintf(stdout, "Node %d: Link %d to %d down at %f:  RTT = %d\n",
		      nodeinfo.address, link, linkstate->states[i].address,
                     timeOfDay(), linkstate->states[i].rtt);

             linkstate->states[i].rtt = 0;
             timeLastResponse[i] = nodeinfo.time_in_usec;
           }
         }
      }*/
      break;

  } /* end switch */
}
/* time to poll our neighbors */

void pollNeighbors() {

 /* DiscoveryPacket p;
  int i;
  int request = Who_R_U;
  DATAGRAM np;

//  extern pLinkStateTable linkstate;


   for each neighbor, send a Poll message
  for (i = 0; i < nodeinfo.nlinks; i++) {
    int link = i+1;                       link number
    p.type = request;                     the request
    p.address = nodeinfo.address;         my address
    p.timestamp = nodeinfo.time_in_usec;  time we send query

    np = allocNetPacket(DISCOVER, nodeinfo.address, ANY_ADDR,
                      (char *) &p, sizeof(p));
    down_to_datalink()


  }*/

/*
#ifndef ADVERTISEMENT
   set number of responses we expect (equals number of links)
  pollCount = linkstate->nlinks;

   start poll fail timer for this poll period
  pollResponseTimer = CNET_start_timer(EV_DISCOVERY_TIMER, DiscoveryTimeOut,
                                       POLLRESPONSETIMER);

   start poll timer for next poll period
  pollTimer = CNET_start_timer(EV_DISCOVERY_TIMER, DiscoveryPeriod, POLLTIMER);
#endif
*/

}