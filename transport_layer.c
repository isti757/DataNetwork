/*
 * transport_layer.c
 *
 *  Created on: Jul 30, 2011
 *      Author: isti
 */
#include <limits.h>

#include "debug.h"
#include "routing.h"
#include "datagram.h"
#include "transport_layer.h"

//-----------------------------------------------------------------------------
// queue containing packets waiting dispatch
QUEUE fragment_queue;
// timers
CnetTimerID flush_queue_timer = NULLTIMER;
static unsigned int packets_retransmitted_total = 0;
//-----------------------------------------------------------------------------
// sliding window definition
typedef struct sliding_window {
    CnetAddr address;
    //-------------------------------------------------------------------------
    QUEUE fragment_queue;
    CnetTimerID flush_queue_timer;
    CnetTimerID separate_ack_timer;
    //-------------------------------------------------------------------------
    // adaptive timeouts
    CnetTime adaptive_timeout;
    CnetTime timesent[NRBUFS][MAXFR];
    //-------------------------------------------------------------------------
    // sliding window variables
    PACKET inpacket[NRBUFS];     // space for assembling incoming packet
    PACKET outpacket[NRBUFS];    // space for storing packets for resending
    uint16_t inlengths[NRBUFS];  // length of incoming packet so far
    uint16_t outlengths[NRBUFS]; // length of outgoing packet
    CnetTimerID timers[NRBUFS];  // retransmit timeout
    int arrived[NRBUFS];         // marks if entire packet arrived

    int nextframetosend;
    int ackexpected;
    int frameexpected;
    int toofar;

    int nbuffered;
    int lastfullfragment;
    int nonack;
    //-------------------------------------------------------------------------
    // fragmentation variables
    uint32_t arrivedacks[NRBUFS][MAXFR];
    uint32_t arrivedfrags[NRBUFS][MAXFR];
    uint16_t mtusize[NRBUFS];

    uint32_t allackssarrived[NRBUFS];

    uint8_t numfrags[NRBUFS];
    uint8_t numacks[NRBUFS];
    int16_t lastfrag[NRBUFS];
} sliding_window;
//-----------------------------------------------------------------------------
// a table of sliding windows
static sliding_window *table;
static int table_size;
//-----------------------------------------------------------------------------
// initializes sliding window table
static void init_sliding_window() {
    table   = (sliding_window *)malloc(sizeof(sliding_window));
    table_size  = 0;
}
//-----------------------------------------------------------------------------
static void destroy_sliding_window() {
    for(int i = 0; i < table_size; i++) {
        queue_free(table[i].fragment_queue);
    }
    free(table);
}
//-----------------------------------------------------------------------------
// finds a sliding window for the corresponding address and returns a
// table index, initializing table entry if neccessary
static int find(CnetAddr address) {
    for(int t = 0; t < table_size ; t++)
    if(table[t].address == address)
        return t;

    table = (sliding_window *)realloc((char *)table, (table_size+1)*sizeof(sliding_window));

    table[table_size].address = address;

    // queue for waiting packets
    table[table_size].fragment_queue = queue_new();
    table[table_size].flush_queue_timer = NULLTIMER;

    table[table_size].separate_ack_timer = NULLTIMER;

    // adaptive timeout
    table[table_size].adaptive_timeout = 0;

    // sliding window
    table[table_size].nextframetosend  = 0;
    table[table_size].ackexpected      = 0;
    table[table_size].frameexpected    = 0;
    table[table_size].toofar           = NRBUFS;
    table[table_size].nbuffered        = 0;
    table[table_size].lastfullfragment = -1;
    table[table_size].nonack           = TRUE;

    for (int b = 0; b < NRBUFS; b++) {
        // sliding window
        table[table_size].arrived[b]    = FALSE;
        table[table_size].timers[b]     = NULLTIMER;
        table[table_size].inlengths[b]  = 0;
        table[table_size].outlengths[b] = 0;
        // fragmentation variables
        table[table_size].allackssarrived[b] = 0;
        table[table_size].numfrags[b]        = 0;
        table[table_size].numacks[b]         = 0;
        table[table_size].mtusize[b]         = 0;
        table[table_size].lastfrag[b]        = -1;

        for (int f = 0; f < MAXFR; f++) {
            table[table_size].timesent[b][f] = -1;
            table[table_size].arrivedacks[b][f] = FALSE;
            table[table_size].arrivedfrags[b][f] = FALSE;
        }
    }

    return (table_size++);
}
//-----------------------------------------------------------------------------
// increment sequence number
static void inc(int *seq) {
    *seq = (*seq + 1) % (MAXSEQ + 1);
}
//-----------------------------------------------------------------------------
// determine if b falls into sliding window
// TODO: optimize checks
static int between(int a, int b, int c) {
    int cond1 = (b < c);
    int cond2 = (c < a);
    int cond3 = (a <= b);
    return ((cond3 && cond1) || (cond2 && cond3) || (cond1 && cond2));
}
//-----------------------------------------------------------------------------
// gives a packet to the network layer setting the piggybacked acknowledgement
// when appropriate
void transmit_packet(uint8_t kind, uint8_t dest, uint16_t packet_len, PACKET packet) {
    int table_ind = find(dest);

    // no need to send a separate ack
    if(table[table_ind].separate_ack_timer != NULLTIMER)
        CNET_stop_timer(table[table_ind].separate_ack_timer);
    table[table_ind].separate_ack_timer = NULLTIMER;

    // figure out where to send
    int link = get_next_link_for_dest(dest);

    // piggyback the acknowledgement
    int last_pkt_ind = table[table_ind].frameexpected;
    if(table[table_ind].lastfrag[last_pkt_ind % NRBUFS] == -1) {
        last_pkt_ind = (last_pkt_ind+MAXSEQ) % (MAXSEQ+1);
        if(table[table_ind].lastfullfragment != -1) {
            kind |= __ACK__;
            packet.ackseqno = last_pkt_ind;
            packet.acksegid = table[table_ind].lastfullfragment;
        }
    } else {
        kind |= __ACK__;
        packet.ackseqno = last_pkt_ind;
        packet.acksegid = table[table_ind].lastfrag[last_pkt_ind % NRBUFS];
    }

    if (is_kind(kind, __ACK__)) {
        T_DEBUG2("i\t\tACK transmitted seq: %d seg: %d\n", packet.ackseqno, packet.acksegid);
    }

    if(is_kind(kind, __NACK__)) {
        table[table_ind].nonack = FALSE;
    }

    // if transmitting data, set up a retransmit timer
    if (is_kind(kind, __DATA__)) {
        uint8_t seqno = packet.seqno % NRBUFS;
        table[table_ind].timesent[seqno][packet.segid] = nodeinfo.time_in_usec;

        if (table[table_ind].adaptive_timeout == 0) {
            double prop_delay = linkinfo[link].propagationdelay;
            double bandwidth = linkinfo[link].bandwidth;
            table[table_ind].adaptive_timeout = 170.0*(prop_delay + 8000000 * ((double)(DATAGRAM_HEADER_SIZE+packet_len) / bandwidth));
        }
        if (table[table_ind].timers[seqno] == NULLTIMER)
            table[table_ind].timers[seqno] = CNET_start_timer(EV_TIMER1, table[table_ind].adaptive_timeout, ((CnetData)(dest << 8) | seqno));

        T_DEBUG3("i\t\tDATA transmitted seq: %d seg: %d len: %d\n", packet.seqno, packet.segid, packet_len-PACKET_HEADER_SIZE);
    }

    write_network(kind, dest, packet_len, (char*)&packet);
}
//-----------------------------------------------------------------------------
// takes a pending packet out of the waiting queue, fragments it and transmits
// fragment by fragment
void flush_queue(CnetEvent ev, CnetTimerID t1, CnetData data) {
    uint8_t src = (uint8_t) data;
    int table_ind = find(src);
    table[table_ind].flush_queue_timer = NULLTIMER;

    if (queue_nitems(table[table_ind].fragment_queue) != 0) {

        T_DEBUG1("Flushing queue %d\n", queue_nitems(table[table_ind].fragment_queue));

        // make sure that the path and mtu are discovered
        size_t frag_len;
        FRAGMENT *frg = queue_peek(table[table_ind].fragment_queue, &frag_len);
        // index of sliding window for the destination
        int table_ind = find(frg->dest);
        int mtu = get_mtu(frg->dest);

        // if path is discovered and there is space in sliding window
        if (mtu != -1) {
            // get the first packet in the queue
            frg = queue_remove(table[table_ind].fragment_queue, &frag_len);

            // copy to a local packet
            size_t total_mess_len = frg->len;
            int nf = table[table_ind].nextframetosend % NRBUFS;

            // copy first packet to the sliding window
            table[table_ind].outlengths[nf] = frg->len;
            memcpy((char*) table[table_ind].outpacket[nf].msg, (char*) frg->pkt.msg, total_mess_len);
            table[table_ind].outpacket[nf].seqno = table[table_ind].nextframetosend;

            // send out all the fragments
            uint32_t segid = 0;
            uint32_t total_length = 0;
            while (total_length < frg->len) {
                PACKET frg_seg;
                // find out the length of the fragment
                uint16_t frg_seg_len;
                if (frg->len - total_length < mtu)
                    frg_seg_len = frg->len - total_length;
                else
                    frg_seg_len = mtu;

                // find out if its last in sequence
                uint8_t kind = __DATA__;
                if (total_length + mtu >= frg->len)
                    kind |= __LASTSGM__;

                // copy part of a message and determine its segment number
                size_t frg_seg_len_t = frg_seg_len;
                memcpy((char*) frg_seg.msg, (char*) frg->pkt.msg + segid * mtu, frg_seg_len_t);
                frg_seg.seqno = table[table_ind].nextframetosend;
                frg_seg.segid = segid;

                transmit_packet(kind, frg->dest, PACKET_HEADER_SIZE + frg_seg_len, frg_seg);

                total_length += frg_seg_len;
                segid++;
            }

            table[table_ind].numacks[nf] = segid;
            inc(&(table[table_ind].nextframetosend));

            free(frg);
        }
        CnetTime timeout = 1;
        table[table_ind].flush_queue_timer = CNET_start_timer(EV_TIMER2, timeout, (CnetData) frg->dest);
    }
    // avoid polling
    else
        table[table_ind].flush_queue_timer = NULLTIMER;

    T_DEBUG1("Flushing queue timer %d\n", table[table_ind].flush_queue_timer);
}
//-----------------------------------------------------------------------------
void handle_ack(uint16_t length, CnetAddr src, PACKET pkt, int table_ind) {

    T_DEBUG2("a\t\tACK arrived seq: %d seg: %d\n", pkt.ackseqno, pkt.acksegid);

    int acked = FALSE;

    // if all fragments arrived and fall into the window
    int pktackseqno = pkt.ackseqno % NRBUFS;
    while (between(table[table_ind].ackexpected, pkt.ackseqno, table[table_ind].nextframetosend)) {

        // avoid recomputing the modulus
        int ackexpected_mod = table[table_ind].ackexpected % NRBUFS;

        // check that all of the fragments are acknowledged
        CnetTime measured, observed;
        if (table[table_ind].ackexpected != pkt.ackseqno) {
            for (int i = 0; i < table[table_ind].numacks[ackexpected_mod]; i++) {
                // handle adaptive timeout
                if(table[table_ind].arrivedacks[ackexpected_mod][i] == FALSE) {
                    measured = table[table_ind].adaptive_timeout;
                    observed = 2*(nodeinfo.time_in_usec-table[table_ind].timesent[ackexpected_mod][i]);
//                    table[table_ind].adaptive_timeout = (1.0-LEARNING_RATE)*measured+LEARNING_RATE*observed;
                    table[table_ind].timesent[ackexpected_mod][i] = -1;
                }
                table[table_ind].arrivedacks[ackexpected_mod][i] = TRUE;
            }
            table[table_ind].allackssarrived[ackexpected_mod] = TRUE;
        } else {
            for (int i = 0; i <= pkt.acksegid; i++) {
                // handle adaptive timeout
                if(table[table_ind].arrivedacks[ackexpected_mod][i] == FALSE) {
                    measured = table[table_ind].adaptive_timeout;
                    observed = 2*(nodeinfo.time_in_usec-table[table_ind].timesent[ackexpected_mod][i]);
//                    table[table_ind].adaptive_timeout = (1.0-LEARNING_RATE)*measured+LEARNING_RATE*observed;
                    table[table_ind].timesent[ackexpected_mod][i] = -1;
                }
                table[table_ind].arrivedacks[ackexpected_mod][i] = TRUE;
            }
            table[table_ind].allackssarrived[pktackseqno] =
                    table[table_ind].arrivedacks[pktackseqno][table[table_ind].numacks[pktackseqno]-1];
        }

        // some fragments stayed unacknowledged
        if(table[table_ind].allackssarrived[ackexpected_mod] == FALSE)
            break;

        // we have acked something so enable the application layer
        acked = TRUE;

        // reset the variables for the next packet
        --table[table_ind].nbuffered;

        for (int i = 0; i < table[table_ind].numacks[ackexpected_mod]; i++) {
            table[table_ind].arrivedacks[ackexpected_mod][i] = FALSE;
            table[table_ind].timesent[ackexpected_mod][i] = -1;
        }

        table[table_ind].numacks[ackexpected_mod] = 0;
        table[table_ind].allackssarrived[ackexpected_mod] = FALSE;

        inc(&(table[table_ind].ackexpected));

        if (table[table_ind].timers[ackexpected_mod] != NULLTIMER)
            CNET_stop_timer(table[table_ind].timers[ackexpected_mod]);

        table[table_ind].timers[ackexpected_mod] = NULLTIMER;
    }
    if (acked && queue_nitems(table[table_ind].fragment_queue) < 10)
        CNET_enable_application(src);
}
//-----------------------------------------------------------------------------
void handle_nack(uint8_t kind, uint16_t length, CnetAddr src, PACKET pkt, int table_ind) {
    int seqno_mod = pkt.seqno % NRBUFS;

    int mtu = get_mtu(src);
    uint32_t total_length = (pkt.segid+1) * mtu;
    int pkt_len = table[table_ind].outlengths[seqno_mod % NRBUFS];

    size_t frg_seg_len;
    if (pkt_len - total_length < mtu)
        frg_seg_len = pkt_len - total_length;
    else
        frg_seg_len = mtu;

    uint8_t frg_seg_kind = __DATA__;
    if (total_length + mtu >= pkt_len)
        frg_seg_kind |= __LASTSGM__;

    PACKET frg;
    memcpy((char*)frg.msg, (char*)table[table_ind].outpacket[seqno_mod].msg + pkt.segid * mtu, frg_seg_len);
    frg.seqno = pkt.seqno;
    frg.segid = pkt.segid;

    transmit_packet(frg_seg_kind, src, PACKET_HEADER_SIZE, frg);
}
//-----------------------------------------------------------------------------
void handle_out_of_order_data(uint16_t length, CnetAddr src, PACKET pkt, int table_ind) {
    PACKET frg;
    if(pkt.seqno != table[table_ind].frameexpected) {
        int prev_frg_id = pkt.segid-1;
        int prev_pkt_seqno = pkt.seqno;
        if (prev_frg_id >= 0) {
            if (table[table_ind].arrivedfrags[prev_pkt_seqno % NRBUFS][prev_frg_id] == FALSE &&
                table[table_ind].nonack) {

                frg.seqno = pkt.seqno;
                frg.segid = prev_frg_id;
                transmit_packet(__NACK__, src, PACKET_HEADER_SIZE, frg);
                return;
            }
        }

        prev_pkt_seqno = (prev_pkt_seqno+MAXSEQ) % (MAXSEQ+1);
        prev_frg_id = table[table_ind].numfrags[prev_pkt_seqno % NRBUFS]-1;
        if (table[table_ind].nonack && prev_frg_id >= 0 &&
            table[table_ind].arrivedfrags[prev_pkt_seqno % NRBUFS][prev_frg_id] == FALSE) {

            frg.seqno = prev_pkt_seqno;
            frg.segid = prev_frg_id;
            transmit_packet(__NACK__, src, PACKET_HEADER_SIZE, frg);
            return;
        }
    } else {
        // if not segment expected
        int prev_frg_id = pkt.segid-1;
        if (prev_frg_id >= 0 && table[table_ind].nonack == TRUE &&
            table[table_ind].arrivedfrags[pkt.seqno % NRBUFS][prev_frg_id] == FALSE) {

            frg.segid = prev_frg_id;
            frg.seqno = pkt.seqno;
            transmit_packet(__NACK__, src, PACKET_HEADER_SIZE, frg);
            return;
        }
    }
}
//-----------------------------------------------------------------------------
void handle_data(uint8_t kind, uint16_t length, CnetAddr src, PACKET pkt, int table_ind) {
    // avoid recomputing the modulus
    int pkt_seqno_mod = pkt.seqno % NRBUFS;

    // figure out mtu and number of fragments to come
    int lastmsg = is_kind(kind, __LASTSGM__);
    if (lastmsg == 0) {
        table[table_ind].mtusize[pkt_seqno_mod] = length - PACKET_HEADER_SIZE;
    }
    if(lastmsg == 1) {
        table[table_ind].numfrags[pkt_seqno_mod] = pkt.segid + 1;
        if (pkt.segid == 0) {
            table[table_ind].mtusize[pkt_seqno_mod] = length - PACKET_HEADER_SIZE;
        }
    }

    // mark fragment arrived and copy content to sliding window
    if (table[table_ind].mtusize[pkt_seqno_mod] != 0) {
        table[table_ind].inpacket[pkt_seqno_mod].seqno = pkt.seqno;
        table[table_ind].arrivedfrags[pkt_seqno_mod][pkt.segid] = TRUE;
        uint32_t offset = table[table_ind].mtusize[pkt_seqno_mod] * pkt.segid;
        size_t mess_len = length - PACKET_HEADER_SIZE;
        memcpy((char*) table[table_ind].inpacket[pkt_seqno_mod].msg + offset, (char*) pkt.msg, mess_len);

        table[table_ind].inlengths[pkt_seqno_mod] += (length - PACKET_HEADER_SIZE);
    }

    // find if all fragments arrived
    if (table[table_ind].numfrags[pkt_seqno_mod] > 0) {
        table[table_ind].arrived[pkt_seqno_mod] = TRUE;
        table[table_ind].lastfrag[pkt_seqno_mod] = -1;
        for (int f = 0; f < table[table_ind].numfrags[pkt_seqno_mod]; f++) {
            if (table[table_ind].arrivedfrags[pkt_seqno_mod][f] == FALSE) {
                table[table_ind].arrived[pkt_seqno_mod] = FALSE;
                break;
            } else {
                table[table_ind].lastfrag[pkt_seqno_mod] = f;
            }
        }
    }
    // deliver arrived messages to application layer
    int arrived_frame = FALSE;
    while (table[table_ind].arrived[table[table_ind].frameexpected % NRBUFS]) {
        arrived_frame = TRUE;
        int frameexpected_mod = table[table_ind].frameexpected % NRBUFS;
        size_t len = table[table_ind].inlengths[frameexpected_mod];

        T_DEBUG2("^\t\tto application len: %llu seq: %d\n", len, table[table_ind].frameexpected);

        // reset the variables for next packet
        table[table_ind].nonack = TRUE;
        table[table_ind].inlengths[frameexpected_mod] = 0;
        table[table_ind].arrived[frameexpected_mod] = FALSE;

        for (int f = 0; f < table[table_ind].numfrags[frameexpected_mod]; f++)
            table[table_ind].arrivedfrags[frameexpected_mod][f] = FALSE;

        table[table_ind].mtusize[frameexpected_mod] = 0;
        table[table_ind].numfrags[frameexpected_mod] = 0;
        table[table_ind].lastfullfragment = table[table_ind].lastfrag[frameexpected_mod];
        table[table_ind].lastfrag[frameexpected_mod] = -1;

        // shift sliding window
        inc(&(table[table_ind].frameexpected));
        inc(&(table[table_ind].toofar));

        // push the message to application layer
        CHECK(CNET_write_application((char*)table[table_ind].inpacket[frameexpected_mod].msg, &len));
    }

    // start a separate ack timer
    if(arrived_frame == TRUE) {
        if(table[table_ind].separate_ack_timer != NULLTIMER)
            CNET_stop_timer(table[table_ind].separate_ack_timer);
        table[table_ind].separate_ack_timer = CNET_start_timer(EV_TIMER3, 100, src);
    }
}
//-----------------------------------------------------------------------------
// called to handle an incoming packet from network layer. based on the type
// of packet carried either marks fragments as acknowledged or fragments
// arrived.
void read_transport(uint8_t kind, uint16_t length, CnetAddr src, char * packet) {
    // copy packet content to a local variable
	PACKET pkt;
	size_t len = length;
	memcpy(&pkt, packet, len);

    // index of the sliding window for the source
    int table_ind = find(src);

    // sender side
    if (is_kind(kind, __ACK__)) {
        if (between(table[table_ind].ackexpected, pkt.ackseqno, table[table_ind].nextframetosend))
            handle_ack(length, src, pkt, table_ind);
    }
    // TODO: fix NACK checking in between
    if (is_kind(kind, __NACK__))
        if(between(table[table_ind].ackexpected, (pkt.ackseqno+1) % (MAXSEQ+1), table[table_ind].nextframetosend)) {
            ;//handle_nack(kind, length, src, pkt, table_ind);
    }
    // receiver side
    if (is_kind(kind, __DATA__)) {
        // if a frame is out of order, possibly a frame loss occured, so send a NACK
        // figure out the previous segment to the one just received and check if it arrived
        //handle_out_of_order_data(length, src, pkt, table_ind);

        T_DEBUG4("i\t\tDATA arrived seq: %d seg: %d bet: %d arr: %d\n", pkt.seqno, pkt.segid,
               between(table[table_ind].frameexpected, pkt.seqno, table[table_ind].toofar),
               table[table_ind].arrivedfrags[pkt.seqno % NRBUFS][pkt.segid]);

        if (between(table[table_ind].frameexpected, pkt.seqno, table[table_ind].toofar) &&
            table[table_ind].arrivedfrags[pkt.seqno % NRBUFS][pkt.segid] == FALSE) {

            handle_data(kind, length, src, pkt, table_ind);
        }
        else
            T_DEBUG2("i\t\tDATA ignored seq: %d seg: %d\n", pkt.seqno, pkt.segid);
    }
}
//-----------------------------------------------------------------------------
// timeout for a separate acknowledgement
void separate_ack_timeout(CnetEvent ev, CnetTimerID t1, CnetData data) {
    uint8_t src = (uint8_t) data;

    int table_ind = find(src);
    table[table_ind].separate_ack_timer = NULLTIMER;

    PACKET frg;
    transmit_packet(__ACK__, src, PACKET_HEADER_SIZE, frg);
}
//-----------------------------------------------------------------------------
// called for retransmitting the timeouted packet. performs fragmentation
// of the packet and transmitting unacknowledged fragments
void ack_timeout(CnetEvent ev, CnetTimerID t1, CnetData data) {

    packets_retransmitted_total++;

    uint8_t seqno = (int) data & UCHAR_MAX;
    uint8_t seqno_mod = seqno % NRBUFS;

    T_DEBUG1("i\t\tDATA timeout seq: %d\n", seqno);

    // index of a sliding window
    int table_ind = find((CnetAddr)((data >> 8) & UCHAR_MAX));
    table[table_ind].timers[seqno_mod] = NULLTIMER;

    // find out where to send and how to fragment
    int mtu = get_mtu(table[table_ind].address);

    // segment id and fraction of packet transmitted
    uint32_t segid = 0;
    uint32_t total_length = 0;

    // total length of a packet
    int pkt_len = table[table_ind].outlengths[table[table_ind].outpacket[seqno_mod].seqno % NRBUFS];

    // fragment timouted packet and transmit
    while (total_length < pkt_len) {
        PACKET frg_seg;

        size_t frg_seg_len;
        if (pkt_len - total_length < mtu)
            frg_seg_len = pkt_len - total_length;
        else
            frg_seg_len = mtu;

        // send only unacknowledged fragments
        if (table[table_ind].arrivedacks[seqno_mod][segid] == FALSE) {
            uint8_t frg_seg_kind = __DATA__;
            if (total_length + mtu >= pkt_len)
                frg_seg_kind |= __LASTSGM__;

            memcpy((char*)frg_seg.msg, (char*)table[table_ind].outpacket[seqno_mod].msg + segid * mtu, frg_seg_len);
            frg_seg.seqno = table[table_ind].outpacket[seqno_mod].seqno;
            frg_seg.segid = segid;

            transmit_packet(frg_seg_kind, table[table_ind].address, PACKET_HEADER_SIZE+frg_seg_len, frg_seg);
        }
        total_length += frg_seg_len;
        segid++;
    }
}
//-----------------------------------------------------------------------------
// read incoming message from application to transport layer
void write_transport(CnetEvent ev, CnetTimerID timer, CnetData data) {
    CnetAddr destaddr;

    FRAGMENT frg;
    frg.len = MAX_MESSAGE_SIZE;

    size_t len = frg.len;
    CHECK(CNET_read_application(&destaddr, frg.pkt.msg, &len));

    frg.len = len;
    frg.kind = __DATA__;
    frg.dest = destaddr;
    frg.pkt.seqno = -1;
    frg.pkt.segid = -1;
    frg.pkt.ackseqno = -1;
    frg.pkt.acksegid = -1;

    size_t frg_len = FRAGMENT_HEADER_SIZE + PACKET_HEADER_SIZE + frg.len;

    int table_ind = find(destaddr);
    if (++table[table_ind].nbuffered == NRBUFS)
        CNET_disable_application(destaddr);

    if(queue_nitems(table[table_ind].fragment_queue) == 0) {
        CnetTime timeout = 1;
        table[table_ind].flush_queue_timer = CNET_start_timer(EV_TIMER2, timeout, (CnetData) destaddr);
    }

    T_DEBUG1("i\tWrite transport %d\n", table[table_ind].flush_queue_timer);


    queue_add(table[table_ind].fragment_queue, &frg, frg_len);
}
//-----------------------------------------------------------------------------
// clean all allocated memory
static void shutdown(CnetEvent ev, CnetTimerID t1, CnetData data) {
    fprintf(stderr, "address: %d\n", nodeinfo.address);
    fprintf(stderr, "\tretransmitted: %d\n", packets_retransmitted_total);
    destroy_sliding_window();
	shutdown_network();
}
//-----------------------------------------------------------------------------
void init_transport() {
	init_sliding_window();

	CHECK(CNET_set_handler( EV_TIMER1, ack_timeout, 0));
	CHECK(CNET_set_handler( EV_TIMER2, flush_queue, 0));
	CHECK(CNET_set_handler( EV_TIMER3, separate_ack_timeout, 0));
	CHECK(CNET_set_handler( EV_SHUTDOWN, shutdown, 0));
	CHECK(CNET_set_handler( EV_APPLICATIONREADY, write_transport, 0));
}
//-----------------------------------------------------------------------------
void signal_transport(SIGNALKIND sg, SIGNALDATA data) {
	if (sg == DISCOVERY_FINISHED) {
		T_DEBUG("Discovery finished\n");
		CNET_enable_application(ALLNODES);
	}
}
//-----------------------------------------------------------------------------
