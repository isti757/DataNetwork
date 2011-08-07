/*
 * discovery.h
 *
 *  Created on: 02.08.2011
 *      Author: kirill
 */
#include "datagram.h"
#include "network_layer.h"
#include "routing.h"
#ifndef DISCOVERY_H_
#define DISCOVERY_H_

void init_discovery();
void do_discovery(int, DATAGRAM dtg);
/* Discovery message format */
typedef struct {
  int type;
  int address;
  CnetTime timestamp;
} DiscoveryPacket,*pDiscoveryPacket;

/* Discovery Type codes */
#define Who_R_U   1
#define I_Am      2
#define Advertise 3
#define MAX_LINKS_COUNT 30

/* alias for cnet Timer Queue Number for Discovery */
#define EV_DISCOVERY_TIMER  EV_TIMER9

/* Discovery protocol timers */
#define DiscoveryStartUp 10       /* first delay for poll = 1 sec in usec 1000000 */
#define DiscoveryTimeOut 30000       /* 0.5 sec in usec for responses to polls*/


#define POLLTIMER          1      /* timer for polling */
#define POLLRESPONSETIMER  2      /* timer for responses to polls */

#endif /* DISCOVERY_H_ */
