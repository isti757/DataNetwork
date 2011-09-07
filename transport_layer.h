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
#include "fragment.h"
#include "network_layer.h"

//-----------------------------------------------------------------------------
// fragmentation
#define MAXPL       (96-DATAGRAM_HEADER_SIZE-PACKET_HEADER_SIZE)
#define MAXFR       (MAX_MESSAGE_SIZE+MAXPL)/MAXPL
// sliding window
#define NBITS       4
#define MAXSEQ      ((1<<NBITS)-1)
#define NRBUFS      (1<<(NBITS-1))

#define FALSE       0
#define TRUE        1
//-----------------------------------------------------------------------------
// adaptive timeouts
#define LEARNING_RATE 0.12500
#define SLOWDOWN_RATE 1.2
//-----------------------------------------------------------------------------
// initialize transport layer
extern void init_transport();
//-----------------------------------------------------------------------------
// read outgoing message from application to transport layer
extern void write_transport(CnetEvent ev, CnetTimerID timer, CnetData data);
//-----------------------------------------------------------------------------
// write incoming message from network to transport
extern void read_transport(uint8_t kind,uint16_t length,CnetAddr source,char * packet);
//-----------------------------------------------------------------------------
extern void signal_transport(SIGNALKIND sg, SIGNALDATA data);
//-----------------------------------------------------------------------------
extern void ack_timeout(CnetEvent ev, CnetTimerID ti, CnetData data);
//-----------------------------------------------------------------------------

#endif /* TRANSPORT_LAYER_H_ */
