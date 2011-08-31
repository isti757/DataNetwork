/*
 * discovery.h
 *
 *  Created on: 02.08.2011
 *      Author: kirill
 */

#ifndef DISCOVERY_H_
#define DISCOVERY_H_

#include <cnet.h>

#include "datagram.h"

//-----------------------------------------------------------------------------
// discovery Type codes
#define Who_R_U   1
#define I_AM      2
#define MAX_LINKS_COUNT 30
//-----------------------------------------------------------------------------
// alias for cnet Timer Queue Number for Discovery
#define EV_DISCOVERY_TIMER  EV_TIMER9
//-----------------------------------------------------------------------------
// discovery protocol timers
#define DISCOVERY_START_UP_TIME 10      // first delay for poll
#define DISCOVERY_TIME_OUT 300000       // 0.5 sec in usec for responses to polls
//-----------------------------------------------------------------------------
#define POLLTIMER          1      // timer for polling
#define POLLRESPONSETIMER  2      // timer for responses to polls
//-----------------------------------------------------------------------------
// discovery message format
typedef struct {
  int type;
  int address;
  CnetTime timestamp;
} __attribute__((packed)) DISCOVERY_PACKET;

#define DISCOVERY_PACKET_SIZE(pkt) (2*sizeof(int)+sizeof(CnetTime))
//-----------------------------------------------------------------------------
extern void init_discovery();
//-----------------------------------------------------------------------------
extern void do_discovery(int, DATAGRAM dtg);
//-----------------------------------------------------------------------------
// poll our neighbors
extern void pollNeighbors();
//-----------------------------------------------------------------------------
extern void timerHandler(CnetEvent, CnetTimerID, CnetData);
//-----------------------------------------------------------------------------

#endif /* DISCOVERY_H_ */
