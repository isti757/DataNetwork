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
#define MAXSEQ      63 //((1<<NBITS)-1)
#define NRBUFS      (1<<(NBITS-1))
// boolean variable
#define FALSE       0
#define TRUE        1
//-----------------------------------------------------------------------------
// adaptive timeouts
#define LEARNING_RATE 0.12500
#define SLOWDOWN_RATE 1.2
#define BETA          LEARNING_RATE
#define ALPHA         (1.0-LEARNING_RATE)
//-----------------------------------------------------------------------------
// sliding window definition
typedef struct sliding_window {
    // statistics
    int messages_processed;
    // refering address
    CnetAddr address;
    // timeouts management
    QUEUE fragment_queue;
    CnetTimerID flush_queue_timer;
    CnetTimerID separate_ack_timer;
    int retransmitted[NRBUFS][MAXFR];
    // adaptive timeouts
    CnetTime adaptive_timeout;
    CnetTime adaptive_deviation;
    CnetTime timesent[NRBUFS][MAXFR];
    // sliding window variables
    int nbuffered;               // sender side
    int ackexpected;
    int nextframetosend;
    int toofar;                  // receiver side
    int nonack;
    int frameexpected;
    int arrived[NRBUFS];         // marks if entire packet arrived
    PACKET inpacket[NRBUFS];     // space for assembling incoming packet
    PACKET outpacket[NRBUFS];    // space for storing packets for resending
    uint16_t inlengths[NRBUFS];  // length of incoming packet so far
    uint16_t outlengths[NRBUFS]; // length of outgoing packet
    CnetTimerID timers[NRBUFS];  // retransmit timeout
    // fragmentation variables
    int lastfullfragment;
    uint8_t numfrags[NRBUFS];
    uint8_t numacks[NRBUFS];
    uint16_t mtusize[NRBUFS];
    int16_t lastfrag[NRBUFS];
    uint32_t allackssarrived[NRBUFS];
    uint32_t arrivedacks[NRBUFS][MAXFR];
    uint32_t arrivedfrags[NRBUFS][MAXFR];
} sliding_window;
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
// private functions:
extern void ack_timeout(CnetEvent ev, CnetTimerID ti, CnetData data);
//-----------------------------------------------------------------------------

#endif /* TRANSPORT_LAYER_H_ */
