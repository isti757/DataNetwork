/*
 * transport_layer.h
 *
 *  Created on: Aug 30, 2011
 *      Author: igor
 */
#ifndef TRANSPORT_LAYER_H_
#define TRANSPORT_LAYER_H_

#include <limits.h>
#include <string.h>

#include "packet.h"
#include "fragment.h"
#include "network_layer.h"
#include "application_layer.h"

//-----------------------------------------------------------------------------
// custom types
typedef int16_t  swin_frag_ind_t;    // maximum fragment segid is < 200
typedef uint8_t  swin_frag_count_t;  // maximum fragment count >= 0
typedef int16_t  swin_mtu_t;         // maximum mtu is < 65535
typedef uint8_t  swin_bool_t;        // boolean type
//-----------------------------------------------------------------------------
// fragmentation
#define MAXPACKLEN    (96-DATAGRAM_HEADER_SIZE-PACKET_HEADER_SIZE)
#define MAXFRAG       (12240+MAXPACKLEN)/MAXPACKLEN
// sliding window
#define NBITS       3
#define MAXSEQ      65535 //USHRT_MAX
#define NRBUFS      (1<<(NBITS-1))
// boolean variable
#define FALSE       0
#define TRUE        1
//-----------------------------------------------------------------------------
// non adaptive timeouts
#define FLUSH_RESTART 1    // 1micro second
#define SEPARATE_ACK_TIMEOUT 10 // 100micro seconds
//-----------------------------------------------------------------------------
// adaptive timeouts
#define ARTIFICIAL_TIMEOUT_LIMIT (10L*60L*60L*1000000L)
#define KARN_CONSTANT 1.5                  // congestion
#define LEARNING_RATE 0.125                // flow
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
    // queue timeouts management
    QUEUE fragment_queue;
    CnetTimerID flush_queue_timer;
    CnetTimerID separate_ack_timer;
    // adaptive and retransmit timeouts
    CnetTime adaptive_timeout;
    CnetTime adaptive_deviation;
    CnetTime timesent[NRBUFS][MAXFRAG];
    CnetTimerID timers[NRBUFS];               // retransmit timeout
    // sliding window variables
    swin_seqno_t nbuffered;                   // sender side
    swin_seqno_t ackexpected;
    swin_seqno_t nextframetosend;
    swin_seqno_t toofar;                      // receiver side
    swin_seqno_t frameexpected;
    // out of order packet management
    swin_bool_t nonack;
    // packet status and reassembly
    swin_bool_t arrived[NRBUFS];                // marks if entire packet arrived
    swin_bool_t retransmitted[NRBUFS][MAXFRAG]; // marks if packet was retransmitted
    PACKET inpacket[NRBUFS];                    // space for assembling incoming packet
    PACKET outpacket[NRBUFS];                   // space for storing packets for resending
    msg_len_t inlengths[NRBUFS];                // length of incoming packet so far
    msg_len_t outlengths[NRBUFS];               // length of outgoing packet
    // fragmentation variables
    swin_mtu_t mtusize[NRBUFS];
    swin_frag_ind_t lastfullfragment;
    swin_frag_count_t numfrags[NRBUFS];
    swin_frag_count_t numacks[NRBUFS];
    swin_frag_ind_t lastfrag[NRBUFS];
    swin_bool_t allackssarrived[NRBUFS];
    swin_bool_t arrivedacks[NRBUFS][MAXFRAG];
    swin_bool_t arrivedfrags[NRBUFS][MAXFRAG];
}  __attribute__((packed)) sliding_window;
//-----------------------------------------------------------------------------
// initialize transport layer
extern void init_transport();
//-----------------------------------------------------------------------------
// read outgoing message from application to transport layer
extern void write_transport(CnetAddr destaddr, char* msg, size_t len);
//-----------------------------------------------------------------------------
// write incoming message from network to transport
extern void read_transport(msg_type_t kind, msg_len_t len, CnetAddr src, char* pkt);
//-----------------------------------------------------------------------------
extern void signal_transport(SIGNALKIND sg, SIGNALDATA data);
//-----------------------------------------------------------------------------
// private functions:
extern void ack_timeout(CnetEvent ev, CnetTimerID ti, CnetData data);
//-----------------------------------------------------------------------------
static void shutdown_transport();
//-----------------------------------------------------------------------------

#endif /* TRANSPORT_LAYER_H_ */
