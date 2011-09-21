/*
 * transport_layer.c
 *
 *  Created on: Jul 30, 2011
 *      Author: isti
 */
#include <limits.h>

#include "cnet.h"
#include "datagram.h"
#include "network_layer.h"
#include "transport_layer.h"
#include "application_layer.h"

//-----------------------------------------------------------------------------
static uint64_t total_memory = 0;
//-----------------------------------------------------------------------------
// statistics
static int reported_messages = 0;
static uint64_t observed_packets = 0;
static uint64_t nacks_handled = 0;
static uint64_t nack_received = 0;
static uint64_t out_of_order = 0;
static CnetTime average_observed = 0;
static CnetTime average_measured = 0;
static CnetTime average_deviation = 0;
static unsigned int packets_retransmitted_total = 0;
static uint64_t fragments_retransmitted_total = 0;
static unsigned int separate_ack = 0;
static unsigned int sent_messages = 0;
//-----------------------------------------------------------------------------
// queue containing packets waiting dispatch
QUEUE fragment_queue;
CnetTimerID flush_queue_timer = NULLTIMER;
//-----------------------------------------------------------------------------
// a table of sliding windows
static sliding_window *swin;
static int swin_size = 0;
//-----------------------------------------------------------------------------
// initializes sliding window table
static void init_sliding_window() {
    swin   = (sliding_window *)malloc(sizeof(sliding_window));
    swin_size  = 0;
}
//-----------------------------------------------------------------------------
static void destroy_sliding_window() {
    total_memory += swin_size*sizeof(sliding_window);
    for(int i = 0; i < swin_size; i++) {
        while(queue_nitems(swin[i].fragment_queue) != 0) {
            size_t el_size;
            FRAGMENT *frg = queue_remove(swin[i].fragment_queue, &el_size);
            total_memory += el_size;
            free(frg);
        }

        queue_free(swin[i].fragment_queue);
    }
    free(swin);
}
//-----------------------------------------------------------------------------
// finds a sliding window for the corresponding address and returns a
// table index, initializing table entry if neccessary
static int find_swin(CnetAddr address) {
    for(int t = 0; t < swin_size ; t++)
        if(swin[t].address == address)
            return t;

    size_t new_size = (swin_size+1)*sizeof(sliding_window);
    swin = (sliding_window *)realloc((char *)swin, new_size);

    swin[swin_size].address = address;

    // queue for waiting packets and timer management
    swin[swin_size].fragment_queue = queue_new();
    swin[swin_size].flush_queue_timer = NULLTIMER;
    swin[swin_size].separate_ack_timer = NULLTIMER;

    // adaptive timeout
    swin[swin_size].adaptive_timeout = 0;
    swin[swin_size].adaptive_deviation = 0;

    // sliding window
    swin[swin_size].nbuffered        = 0;
    swin[swin_size].ackexpected      = 0;
    swin[swin_size].nextframetosend  = 0;
    swin[swin_size].toofar           = NRBUFS;
    swin[swin_size].nonack           = TRUE;
    swin[swin_size].frameexpected    = 0;
    swin[swin_size].lastfullfragment = -1;

    for (int b = 0; b < NRBUFS; b++) {        
        // sliding window
        swin[swin_size].arrived[b]    = FALSE;
        swin[swin_size].timers[b]     = NULLTIMER;
        swin[swin_size].inlengths[b]  = 0;
        swin[swin_size].outlengths[b] = 0;
        // fragmentation variables
        swin[swin_size].allackssarrived[b] = 0;
        swin[swin_size].numfrags[b]        = 0;
        swin[swin_size].numacks[b]         = 0;
        swin[swin_size].mtusize[b]         = 0;
        swin[swin_size].lastfrag[b]        = -1;

        for (int f = 0; f < MAXFR; f++) {
            // adaptive timeout management
            swin[swin_size].retransmitted[b][f] = FALSE;
            swin[swin_size].timesent[b][f] = -1;
            // marks arrived acks and fragments
            swin[swin_size].arrivedacks[b][f] = FALSE;
            swin[swin_size].arrivedfrags[b][f] = FALSE;
        }
    }

    return (swin_size++);
}
//-----------------------------------------------------------------------------
// increment sequence number
static void inc(swin_seqno_t *seq) {
    *seq = (*seq + 1) % (MAXSEQ + 1);
}
//-----------------------------------------------------------------------------
// determine if b falls into sliding window
static int between(swin_seqno_t a, swin_seqno_t b, swin_seqno_t c) {
    int cond1 = (b < c);
    int cond2 = (c < a);
    int cond3 = (a <= b);
    return ((cond3 && cond1) || (cond2 && cond3) || (cond1 && cond2));
}
//-----------------------------------------------------------------------------
// reset sliding window variables on sender side
static void reset_sender_window(swin_seqno_t ack_mod, int table_ind) {
    --swin[table_ind].nbuffered;
    for (int i = 0; i < (int)swin[table_ind].numacks[ack_mod]; i++) {
        swin[table_ind].retransmitted[ack_mod][i] = FALSE;
        swin[table_ind].arrivedacks[ack_mod][i] = FALSE;
        swin[table_ind].timesent[ack_mod][i] = -1;
    }
    swin[table_ind].numacks[ack_mod] = 0;
    swin[table_ind].allackssarrived[ack_mod] = FALSE;
    if (swin[table_ind].timers[ack_mod] != NULLTIMER)
        CNET_stop_timer(swin[table_ind].timers[ack_mod]);
    swin[table_ind].timers[ack_mod] = NULLTIMER;
}
//-----------------------------------------------------------------------------
// reset sliding window variables on receiver side
static void reset_receiver_window(swin_seqno_t frameexpected_mod, int table_ind) {
    swin[table_ind].nonack = TRUE;
    swin[table_ind].inlengths[frameexpected_mod] = 0;
    swin[table_ind].arrived[frameexpected_mod] = FALSE;

    for (int f = 0; f < swin[table_ind].numfrags[frameexpected_mod]; f++)
        swin[table_ind].arrivedfrags[frameexpected_mod][f] = FALSE;

    swin[table_ind].mtusize[frameexpected_mod] = 0;
    swin[table_ind].numfrags[frameexpected_mod] = 0;
    swin[table_ind].lastfullfragment = swin[table_ind].lastfrag[frameexpected_mod];
    swin[table_ind].lastfrag[frameexpected_mod] = -1;
}
//-----------------------------------------------------------------------------
// set a retransmit timer for a packet
static void set_timeout(swin_addr_t dst, msg_len_t pkt_len, PACKET pkt, int table_ind) {
    swin_seqno_t seqno = pkt.seqno % NRBUFS;

    // figure out where to send
    int link = get_next_link_for_dest(dst);

    // initialize timeout
    if (swin[table_ind].adaptive_timeout == 0) {
        // propagation delay
        double prop_delay = get_propagation_delay(dst);

        // transmission delay
        swin_frag_count_t frag_num = swin[table_ind].numacks[seqno];
        msg_len_t data_len = frag_num*(PACKET_HEADER_SIZE+DATAGRAM_HEADER_SIZE+pkt_len);
        double bandwidth = linkinfo[link].bandwidth;
        double trans_delay = 8000000 * ((double)(data_len) / bandwidth);
        swin[table_ind].adaptive_timeout = 3.0*(prop_delay + trans_delay);
    }
    // set timeout based on first fragment only
    if (swin[table_ind].timers[seqno] == NULLTIMER) {
        CnetData data = (dst << SEQNO_WIDTH) | seqno;
        CnetTime timeout = 2*swin[table_ind].adaptive_timeout+
                4*swin[table_ind].adaptive_deviation;
        swin[table_ind].timers[seqno] = CNET_start_timer(EV_TIMER1, timeout, data);
    }
}
//-----------------------------------------------------------------------------
// gives a packet to the network layer setting the piggybacked acknowledgement
// when appropriate
void transmit_packet(msg_type_t kind, swin_addr_t dst, msg_len_t pkt_len, PACKET pkt) {
    int table_ind = find_swin(dst);

    // no need to send a separate ack
    if(swin[table_ind].separate_ack_timer != NULLTIMER)
        CNET_stop_timer(swin[table_ind].separate_ack_timer);
    swin[table_ind].separate_ack_timer = NULLTIMER;

    // piggyback the acknowledgement
    swin_seqno_t last_pkt_ind = swin[table_ind].frameexpected;
    if(swin[table_ind].lastfrag[last_pkt_ind % NRBUFS] == -1) {
        last_pkt_ind = (last_pkt_ind+MAXSEQ) % (MAXSEQ+1);
        if(swin[table_ind].lastfullfragment > -1) {
            kind |= __ACK__;
            pkt.ackseqno = last_pkt_ind;
            pkt.acksegid = swin[table_ind].lastfullfragment;
        }
    } else {
        kind |= __ACK__;
        pkt.ackseqno = last_pkt_ind;
        pkt.acksegid = swin[table_ind].lastfrag[last_pkt_ind % NRBUFS];
    }

    if(is_kind(kind, __NACK__)) {
        swin[table_ind].nonack = FALSE;
    }

    // if transmitting data, set up a retransmit timer
    if (is_kind(kind, __DATA__))
        set_timeout(dst, pkt_len, pkt, table_ind);

    write_network(kind, dst, pkt_len, (char*)&pkt);
}
//-----------------------------------------------------------------------------
// takes a pending packet out of the waiting queue, fragments it and transmits
// fragment by fragment
void flush_queue(CnetEvent ev, CnetTimerID t1, CnetData data) {
    // find host and appropriate sliding window
    swin_addr_t dest = (swin_addr_t) data;
    int table_ind = find_swin(dest);
    swin[table_ind].flush_queue_timer = NULLTIMER;

    if (queue_nitems(swin[table_ind].fragment_queue) != 0) {
        size_t frag_len;
        FRAGMENT *frg = queue_peek(swin[table_ind].fragment_queue, &frag_len);

        // make sure that the path and mtu are discovered
        swin_mtu_t mtu = get_mtu(frg->dest, TRUE);
        if(mtu == -1) {
             CNET_disable_application(dest);
             swin[table_ind].flush_queue_timer = NULLTIMER;
             return;
        }
        // if path is discovered and there is space in sliding window
        if (mtu != -1) {
            sent_messages++;

            // get the first packet in the queue
            swin_seqno_t nf = swin[table_ind].nextframetosend % NRBUFS;
            frg = queue_remove(swin[table_ind].fragment_queue, &frag_len);

            // copy first packet to the sliding window
            size_t total_mess_len = frg->len;
            swin[table_ind].outlengths[nf] = frg->len;
            swin[table_ind].outpacket[nf].seqno = swin[table_ind].nextframetosend;
            memcpy((char*) swin[table_ind].outpacket[nf].msg,
                   (char*) frg->pkt.msg, total_mess_len);

            // send out all the fragments
            swin_segid_t segid = 0;
            msg_len_t total_length = 0;
            while (total_length < frg->len) {
                PACKET frg_seg;
                msg_len_t frg_seg_len;
                if (frg->len - total_length < mtu)
                    frg_seg_len = frg->len - total_length;
                else
                    frg_seg_len = mtu;

                // find out if its last in sequence
                msg_type_t kind = __DATA__;
                if (total_length + mtu >= frg->len)
                    kind |= __LASTSGM__;

                // copy part of a message and determine its segment number
                size_t frg_seg_len_t = frg_seg_len;
                memcpy((char*) frg_seg.msg,
                       (char*) frg->pkt.msg+segid*mtu, frg_seg_len_t);
                frg_seg.seqno = swin[table_ind].nextframetosend;
                frg_seg.segid = segid;

                // timestamp of a fragment
                swin[table_ind].timesent[nf][frg_seg.segid] = nodeinfo.time_in_usec;

                frg_seg_len_t += PACKET_HEADER_SIZE;
                transmit_packet(kind, frg->dest, frg_seg_len_t, frg_seg);

                total_length += frg_seg_len;
                segid++;
            }

            swin[table_ind].numacks[nf] = segid;
            inc(&(swin[table_ind].nextframetosend));

            free(frg);
        }
    }
    // callback the flush_queue function
    if (queue_nitems(swin[table_ind].fragment_queue) != 0)
        swin[table_ind].flush_queue_timer =
                CNET_start_timer(EV_TIMER2, FLUSH_RESTART, dest);
    else
        swin[table_ind].flush_queue_timer = NULLTIMER;
}
//-----------------------------------------------------------------------------
// handle incoming acknowledgement
void handle_ack(CnetAddr src, PACKET pkt, int table_ind) {
    swin_bool_t acked = FALSE;

    // if all fragments arrived and fall into the window
    swin_seqno_t pktackseqno = pkt.ackseqno % NRBUFS;
    while (between(swin[table_ind].ackexpected, pkt.ackseqno, swin[table_ind].nextframetosend)) {
        // avoid recomputing the modulus
        swin_seqno_t ack_mod = swin[table_ind].ackexpected % NRBUFS;

        // check that all of the fragments are acknowledged
        CnetTime measured, curr_deviation, observed, difference;
        if (swin[table_ind].ackexpected != pkt.ackseqno) {
            // handle adaptive timeout
            for (int i = 0; i < (int)swin[table_ind].numacks[ack_mod]; i++) {
                if((swin[table_ind].arrivedacks[ack_mod][i] == FALSE) &&
                   (swin[table_ind].retransmitted[ack_mod][i] == FALSE)) {
                    // update timeout
                    measured = swin[table_ind].adaptive_timeout;
                    observed = nodeinfo.time_in_usec-swin[table_ind].timesent[ack_mod][i];

                    // update standard deviation
                    difference = observed-measured;
                    curr_deviation = swin[table_ind].adaptive_deviation;
                    swin[table_ind].adaptive_deviation = ALPHA*curr_deviation+ALPHA*fabs(difference);
                    swin[table_ind].adaptive_timeout = ALPHA*measured+ALPHA*observed;

                    // update statistics
                    observed_packets++;
                    average_measured += swin[table_ind].adaptive_timeout;
                    average_observed += observed;
                    average_deviation += swin[table_ind].adaptive_deviation;
                }
                swin[table_ind].timesent[ack_mod][i] = -1;
                swin[table_ind].arrivedacks[ack_mod][i] = TRUE;
            }
            swin[table_ind].allackssarrived[ack_mod] = TRUE;
        } else {
            for (int i = 0; i <= (int)pkt.acksegid; i++) {
                if((swin[table_ind].arrivedacks[ack_mod][i] == FALSE) &&
                   (swin[table_ind].retransmitted[ack_mod][i] == FALSE)) {
                    // update timeout
                    measured = swin[table_ind].adaptive_timeout;
                    observed = nodeinfo.time_in_usec-swin[table_ind].timesent[ack_mod][i];

                    // update standard deviation
                    difference = observed-measured;
                    curr_deviation = swin[table_ind].adaptive_deviation;
                    swin[table_ind].adaptive_deviation = ALPHA*curr_deviation+BETA*fabs(difference);
                    swin[table_ind].adaptive_timeout = ALPHA*measured+BETA*observed;

                    // update statistics
                    observed_packets++;
                    average_measured += swin[table_ind].adaptive_timeout;
                    average_observed += nodeinfo.time_in_usec-swin[table_ind].timesent[ack_mod][i];
                    average_deviation += swin[table_ind].adaptive_deviation;
                }
                swin[table_ind].timesent[ack_mod][i] = -1;
                swin[table_ind].arrivedacks[ack_mod][i] = TRUE;
            }
            swin_frag_count_t num_acks = swin[table_ind].numacks[pktackseqno];
            swin[table_ind].allackssarrived[pktackseqno] =
                    swin[table_ind].arrivedacks[pktackseqno][num_acks-1];
        }

        // some fragments stayed unacknowledged
        if(swin[table_ind].allackssarrived[ack_mod] == FALSE)
            break;

        // we have acked something so enable the application layer
        acked = TRUE;

        // reset the variables for the next packet
        reset_sender_window(ack_mod, table_ind);

        inc(&(swin[table_ind].ackexpected));
    }
    if (acked && queue_nitems(swin[table_ind].fragment_queue) < 10)
        CHECK(CNET_enable_application(src));
}
//-----------------------------------------------------------------------------
// handle an incoming negative acknowledgement
void handle_nack(CnetAddr src, int table_ind) {
    nacks_handled++;
    swin_seqno_t seqno = swin[table_ind].ackexpected % NRBUFS;

    if(swin[table_ind].timers[seqno] != NULLTIMER)
        CHECK(CNET_stop_timer(swin[table_ind].timers[seqno]));
    swin[table_ind].timers[seqno] = NULLTIMER;

    // will be multiplied by Karn's constant in ack_timeout
    swin[table_ind].adaptive_timeout /= KARN_CONSTANT;

    CnetData data = (src << SEQNO_WIDTH) | seqno;
    ack_timeout(-1, -1, data);
}
//-----------------------------------------------------------------------------
// handle data out of order (possibly a loss occured)
void handle_out_of_order_data(CnetAddr src, PACKET pkt, int table_ind) {
    // if not the segment expected send a NACK
    PACKET frg;
    if(pkt.seqno != swin[table_ind].frameexpected && swin[table_ind].nonack) {
        swin_frag_ind_t frg_id = pkt.segid-1;
        if(frg_id == -1) {
            frg.seqno = (pkt.seqno+MAXSEQ)%(MAXSEQ+1);
        } else
            frg.seqno = pkt.seqno;

        out_of_order++;

        frg.segid = 0;
        transmit_packet(__NACK__, src, PACKET_HEADER_SIZE, frg);
    } else {
        swin_frag_ind_t frg_id = pkt.segid-1;
        if (swin[table_ind].nonack && frg_id >= 0 &&
            swin[table_ind].arrivedfrags[pkt.seqno % NRBUFS][frg_id] == FALSE) {

            out_of_order++;

            frg.seqno = pkt.seqno;
            frg.segid = frg_id;
            transmit_packet(__NACK__, src, PACKET_HEADER_SIZE, frg);
        }
    }
}
//-----------------------------------------------------------------------------
// handle incoming data
void handle_data(msg_type_t kind, msg_len_t length, CnetAddr src, PACKET pkt, int table_ind) {
    // avoid recomputing the modulus
    swin_seqno_t pkt_seqno_mod = pkt.seqno % NRBUFS;

    // figure out mtu and number of fragments to come
    swin_bool_t lastmsg = is_kind(kind, __LASTSGM__);
    if (lastmsg == 0)
        swin[table_ind].mtusize[pkt_seqno_mod] = length-PACKET_HEADER_SIZE;
    if(lastmsg == 1) {
        swin[table_ind].numfrags[pkt_seqno_mod] = pkt.segid + 1;
        if (pkt.segid == 0) {
            swin[table_ind].mtusize[pkt_seqno_mod] = length-PACKET_HEADER_SIZE;
        }
    }

    // mark fragment arrived and copy content to sliding window buffer
    if (swin[table_ind].mtusize[pkt_seqno_mod] != 0) {
        swin[table_ind].inpacket[pkt_seqno_mod].seqno = pkt.seqno;
        swin[table_ind].arrivedfrags[pkt_seqno_mod][pkt.segid] = TRUE;

        size_t mess_len = length-PACKET_HEADER_SIZE;
        uint64_t offset = swin[table_ind].mtusize[pkt_seqno_mod]*pkt.segid;
        memcpy((char*)swin[table_ind].inpacket[pkt_seqno_mod].msg+offset,
               (char*) pkt.msg, mess_len);

        swin[table_ind].inlengths[pkt_seqno_mod] += (length - PACKET_HEADER_SIZE);
    }

    // find if all fragments arrived
    swin_frag_ind_t prev_last_frag = swin[table_ind].lastfrag[pkt_seqno_mod];
    if (swin[table_ind].numfrags[pkt_seqno_mod] > 0) {
        swin[table_ind].arrived[pkt_seqno_mod] = TRUE;
        swin[table_ind].lastfrag[pkt_seqno_mod] = -1;

        //int num_frags = swin[table_ind].numfrags[pkt_seqno_mod];
        for (int f = 0; f < swin[table_ind].numfrags[pkt_seqno_mod]; f++) {
            if (swin[table_ind].arrivedfrags[pkt_seqno_mod][f] == FALSE) {
                swin[table_ind].arrived[pkt_seqno_mod] = FALSE;
                break;
            } else {
                swin[table_ind].lastfrag[pkt_seqno_mod] = f;
            }
        }
    }

    // deliver arrived messages to application layer
    swin_bool_t arrived_frame = FALSE;
    while (swin[table_ind].arrived[swin[table_ind].frameexpected % NRBUFS]) {
        arrived_frame = TRUE;
        swin_seqno_t frameexpected_mod = swin[table_ind].frameexpected % NRBUFS;
        size_t len = swin[table_ind].inlengths[frameexpected_mod];

        // push the message to application layer
        read_application((char*)swin[table_ind].inpacket[frameexpected_mod].msg, len);

        // reset the variables for next packet
        reset_receiver_window(frameexpected_mod, table_ind);

        // shift sliding window
        inc(&(swin[table_ind].frameexpected));
        inc(&(swin[table_ind].toofar));

        reported_messages++;
    }

    // start a separate ack timer
    if(arrived_frame == TRUE || prev_last_frag < swin[table_ind].lastfrag[pkt_seqno_mod]) {
        if(swin[table_ind].separate_ack_timer != NULLTIMER)
            CHECK(CNET_stop_timer(swin[table_ind].separate_ack_timer));
        swin[table_ind].separate_ack_timer =
                CNET_start_timer(EV_TIMER3, SEPARATE_ACK_TIMEOUT, src);
    }
}
//-----------------------------------------------------------------------------
// called to handle an incoming packet from network layer. based on the type
// of packet carried either marks fragments as acknowledged or fragments
// arrived.
void read_transport(msg_type_t kind, msg_len_t length, CnetAddr src, char * packet) {
    // copy packet content to a local variable
    PACKET pkt;
    size_t len = length;
    memcpy(&pkt, packet, len);

    // index of the sliding window for the source
    int table_ind = find_swin(src);

    // sender side
    if (is_kind(kind, __ACK__)) {
        if (between(swin[table_ind].ackexpected, pkt.ackseqno, swin[table_ind].nextframetosend))
            handle_ack(src, pkt, table_ind);
    }
    if (is_kind(kind, __NACK__)) {
        handle_nack(src, table_ind);
        nack_received++;
    }
    // receiver side
    if (is_kind(kind, __DATA__)) {
        // if a frame is out of order, possibly a frame loss occured, so send a NACK
        // figure out the previous segment to the one just received and check if it arrived
        handle_out_of_order_data(src, pkt, table_ind);

        if (between(swin[table_ind].frameexpected, pkt.seqno, swin[table_ind].toofar) &&
            swin[table_ind].arrivedfrags[pkt.seqno % NRBUFS][pkt.segid] == FALSE) {

            handle_data(kind, length, src, pkt, table_ind);
        }
    }
}
//-----------------------------------------------------------------------------
// timeout for a specific packet
void separate_ack_timeout(CnetEvent ev, CnetTimerID t1, CnetData data) {
    separate_ack++;

    swin_addr_t src = (swin_addr_t) data;
    int table_ind = find_swin(src);
    swin[table_ind].separate_ack_timer = NULLTIMER;

    PACKET frg;
    transmit_packet(__ACK__, src, PACKET_HEADER_SIZE, frg);
}
//-----------------------------------------------------------------------------
// called for retransmitting the timeouted packet. performs fragmentation
// of the packet and transmitting unacknowledged fragments
void ack_timeout(CnetEvent ev, CnetTimerID t1, CnetData data) {
    packets_retransmitted_total++;

    swin_seqno_t seqno = data & SEQNO_MASK;
    swin_seqno_t seqno_mod = seqno % NRBUFS;

    // index of a sliding window
    CnetAddr addr = (CnetAddr)((data >> SEQNO_WIDTH) & ADDR_MASK);
    int table_ind = find_swin(addr);
    swin[table_ind].timers[seqno_mod] = NULLTIMER;

    if(swin[table_ind].adaptive_timeout < 360000000)
        // Karn's algorithm
        swin[table_ind].adaptive_timeout *= KARN_CONSTANT;

    // find out where to send and how to fragment
    swin_mtu_t mtu = get_mtu(swin[table_ind].address, FALSE);

    // segment id and fraction of packet transmitted
    swin_frag_ind_t segid = 0;
    msg_len_t total_length = 0;

    // total length of a packet
    msg_len_t msg_len = swin[table_ind].outlengths[seqno_mod];

    // fragment timouted packet and transmit
    while (total_length < msg_len) {
        PACKET frg_seg;

        size_t frg_seg_len;
        if (msg_len - total_length < mtu)
            frg_seg_len = msg_len - total_length;
        else
            frg_seg_len = mtu;

        // send only unacknowledged fragments
        if (swin[table_ind].arrivedacks[seqno_mod][segid] == FALSE) {
            fragments_retransmitted_total++;
            msg_type_t frg_seg_kind = __DATA__;
            if (total_length + mtu >= msg_len)
                frg_seg_kind |= __LASTSGM__;

            memcpy((char*)frg_seg.msg,
                   (char*)swin[table_ind].outpacket[seqno_mod].msg+segid*mtu, frg_seg_len);
            frg_seg.seqno = swin[table_ind].outpacket[seqno_mod].seqno;
            frg_seg.segid = segid;

            swin[table_ind].retransmitted[frg_seg.seqno % NRBUFS][frg_seg.segid] = TRUE;
            transmit_packet(frg_seg_kind, swin[table_ind].address,
                            PACKET_HEADER_SIZE+frg_seg_len, frg_seg);
        }
        total_length += frg_seg_len;
        segid++;
    }
}
//-----------------------------------------------------------------------------
// read incoming message from application to transport layer and put it into
// the waiting queue, perform compression
void write_transport(CnetAddr destaddr, char* msg, size_t len) {
    FRAGMENT frg;
    memcpy((char*)frg.pkt.msg, (char*)msg, len);

    // initialize a fragment
    frg.len = len;
    frg.kind = __DATA__;
    frg.dest = destaddr;
    frg.pkt.seqno = -1;
    frg.pkt.segid = -1;
    frg.pkt.ackseqno = -1;
    frg.pkt.acksegid = -1;

    // find corresponding sliding window
    int table_ind = find_swin(destaddr);
    if (++swin[table_ind].nbuffered == NRBUFS)
        CHECK(CNET_disable_application(destaddr));

    if(queue_nitems(swin[table_ind].fragment_queue) == 0)
        swin[table_ind].flush_queue_timer =
                CNET_start_timer(EV_TIMER2, FLUSH_RESTART, destaddr);

    size_t frg_len = FRAGMENT_HEADER_SIZE+PACKET_HEADER_SIZE+frg.len;
    queue_add(swin[table_ind].fragment_queue, &frg, frg_len);
}
//-----------------------------------------------------------------------------
void init_transport() {
    init_sliding_window();

    CHECK(CNET_set_handler( EV_TIMER1, ack_timeout, 0));
    CHECK(CNET_set_handler( EV_TIMER2, flush_queue, 0));
    CHECK(CNET_set_handler( EV_TIMER3, separate_ack_timeout, 0));
}
//-----------------------------------------------------------------------------
void signal_transport(SIGNALKIND sg, SIGNALDATA data) {
    if (sg == DISCOVERY_FINISHED) {
        CNET_enable_application(ALLNODES);
    }
    if (sg == MTU_DISCOVERED) {
        swin_addr_t address = (swin_addr_t)data;
        int table_ind = find_swin(address);
        swin[table_ind].flush_queue_timer =
                CNET_start_timer(EV_TIMER2, 1, (CnetData) address);
        CNET_enable_application(address);
    }
}
//-----------------------------------------------------------------------------
void shutdown_transport() {
    destroy_sliding_window();
    fprintf(stderr, "address: %d\n", nodeinfo.address);
    fprintf(stderr, "\ttotal sent: %d\n", sent_messages);
    fprintf(stderr, "\tretransmitted pack: %d\n", packets_retransmitted_total);
    fprintf(stderr, "\tretransmitted frag: %lu\n", fragments_retransmitted_total);
    fprintf(stderr, "\tseparate ack: %d\n", separate_ack);
    fprintf(stderr, "\tnacks handled: %d\n", (int)nacks_handled);
    fprintf(stderr, "\tout of order: %d\n", (int)out_of_order);
    fprintf(stderr, "\tnacks received: %d\n", (int)nack_received);
    fprintf(stderr, "\tdeviation timer: %f\n",((double)average_deviation/(double)observed_packets));
    fprintf(stderr, "\tmeasured timer: %f\n", ((double)average_measured / (double) observed_packets));
    fprintf(stderr, "\tobserved timer: %f\n", ((double)average_observed / (double) observed_packets));
    fprintf(stderr, "\ttransport memory: %f(MB)\n", (double)total_memory / (double)(8*1024*1024));
    fprintf(stderr, "\tsliding window mem: %lu\n", sizeof(sliding_window));
    shutdown_network();
}
//-----------------------------------------------------------------------------
