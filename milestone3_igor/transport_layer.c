/*
 * transport_layer.c
 *
 *  Created on: Jul 30, 2011
 *      Author: isti
 */

#include <limits.h>
#include <stdint.h>
#include <assert.h>

#include "transport_layer.h"

//-----------------------------------------------------------------------------
// queue containing packets waiting dispatch
QUEUE fragment_queue;
// timers
CnetTimerID flush_queue_timer = NULLTIMER;
CnetTimerID separate_ack_timer = NULLTIMER;
//-----------------------------------------------------------------------------
// sliding window variables
static PACKET inpacket[NRBUFS], outpacket[NRBUFS];
static uint16_t inlengths[NRBUFS], outlengths[NRBUFS];

static int arrived[NRBUFS];
static CnetAddr dests[NRBUFS];
static CnetTimerID timers[NRBUFS];

static int nextframetosend = 0;
static int ackexpected = 0;
static int frameexpected = 0;
static int toofar = NRBUFS;

static int nbuffered = 0;
static int lastfullfragment = -1;
//-----------------------------------------------------------------------------
// fragmentation variables
static uint32_t arrivedacks[NRBUFS][MAXFR];
static uint32_t arrivedfrags[NRBUFS][MAXFR];
static uint16_t mtusize[NRBUFS];

static uint32_t allackssarrived[NRBUFS];

static int8_t numfrags[NRBUFS];
static int8_t numacks[NRBUFS];
static int8_t lastfrag[NRBUFS];
//-----------------------------------------------------------------------------
// increment sequence number
static void inc(int *seq) {
    *seq = (*seq + 1) % (MAXSEQ + 1);
}
//-----------------------------------------------------------------------------
// determine if b falls into sliding window
// TODO: optimize checks
static int between(int a, int b, int c) {
    return (((a <= b) && (b < c)) ||
            ((c < a) && (a <= b)) ||
            ((b < c) && (c < a)));
}
//-----------------------------------------------------------------------------
// kind of packet
static int is_kind(uint8_t kind, uint8_t knd) {
    return ((kind & knd) > 0);
}
//-----------------------------------------------------------------------------
// gives a packet to the network layer setting the piggybacked acknowledgement
// when appropriate
void transmit_packet(uint8_t kind, CnetAddr dest, uint16_t packet_len,  PACKET packet) {

    // no need to send a separate ack
    if(separate_ack_timer != NULLTIMER)
        CNET_stop_timer(separate_ack_timer);
    separate_ack_timer = NULLTIMER;

    // figure out where to send
    int link = get_next_link_for_dest(dest);

    // piggyback the acknowledgement
    int last_frg_ind = frameexpected;
    if(lastfrag[last_frg_ind % NRBUFS] == -1) {
        last_frg_ind = (last_frg_ind+MAXSEQ) % (MAXSEQ+1);
        if(lastfullfragment != -1) {
            kind |= __ACK__;
            packet.ackseqno = last_frg_ind;
            packet.acksegid = lastfullfragment;
        }
    }
    else
    {
        kind |= __ACK__;
        packet.ackseqno = last_frg_ind;
        packet.acksegid = lastfrag[last_frg_ind % NRBUFS];
    }

    if (is_kind(kind, __ACK__)) {
        printf("i\t\tACK transmitted seq: %d seg: %d\n", packet.ackseqno, packet.acksegid);
    }

    // if transmitting data, set up a retransmit timer
    if (is_kind(kind, __DATA__)) {
        uint8_t seqno = packet.seqno;

        dests[seqno % NRBUFS] = dest;

        CnetTime timeout;
        float prop_delay = linkinfo[link].propagationdelay;
        timeout = 2.5 * (prop_delay + 8000000 * ((float)(DATAGRAM_HEADER_SIZE+packet_len) / prop_delay));

        if(timers[seqno % NRBUFS] == NULLTIMER)
            timers[seqno % NRBUFS] = CNET_start_timer(EV_TIMER1, timeout, (CnetData) seqno);

        printf("i\t\tDATA transmitted seq: %d seg: %d len: %d\n", packet.seqno, packet.segid, packet_len-PACKET_HEADER_SIZE);
    }

    write_network(kind, dest, packet_len, packet);
}
//-----------------------------------------------------------------------------
// takes a pending packet out of the waiting queue, fragments it and transmits
// fragment by fragment
void flush_queue(CnetEvent ev, CnetTimerID t1, CnetData data) {

    if (queue_nitems(fragment_queue) != 0) {
        // make sure that the path and mtu are discovered
        size_t frag_len;
        FRAGMENT *frg = queue_peek(fragment_queue, &frag_len);
        int link = get_next_link_for_dest(frg->dest);
        int mtu = get_mtu_for_link(link);

        if (link != -1 && mtu != -1) {
            // get the first packet in the queue
            frg = queue_remove(fragment_queue, &frag_len);

            // copy to a local packet
            size_t total_mess_len = frg->len;

            int nf = nextframetosend % NRBUFS;
            if (++nbuffered == NRBUFS)
                CNET_disable_application(ALLNODES);

            // copy first packet to the sliding window
            outlengths[nf] = frg->len;
            memcpy((char*) outpacket[nf].msg, (char*) frg->pkt.msg, total_mess_len);
            outpacket[nf].seqno = nextframetosend;

            // send out all the fragments
            uint32_t segid = 0;
            uint32_t total_length = 0;
            while (total_length < frg->len) {

                PACKET frg_seg;
                uint16_t frg_seg_len;

                // find out the length of the fragment
                if (frg->len - total_length < mtu)
                    frg_seg_len = frg->len - total_length;
                else
                    frg_seg_len = mtu;

                uint8_t kind = __DATA__;
                if (total_length + mtu >= frg->len) {
                    kind |= __LASTSGM__;
                }

                size_t frg_seg_len_t = frg_seg_len;
                memcpy((char*) frg_seg.msg, (char*) frg->pkt.msg + segid * mtu,
                        frg_seg_len_t);
                frg_seg.seqno = nextframetosend;
                frg_seg.segid = segid;

                transmit_packet(kind, frg->dest, PACKET_HEADER_SIZE + frg_seg_len, frg_seg);

                total_length += frg_seg_len;
                segid++;
            }

            numacks[nf] = segid;
            inc(&nextframetosend);

            free(frg);
        }
        CnetTime timeout = 1;
        flush_queue_timer = CNET_start_timer(EV_TIMER2, timeout, (CnetData) -1);
    }

    else
        flush_queue_timer = NULLTIMER;
}
//-----------------------------------------------------------------------------
// called to handle an incoming packet from network layer. based on the type
// of packet carried either marks fragments as acknowledged or fragments
// arrived.
void read_transport(uint8_t kind, uint16_t length, CnetAddr src, PACKET pkt) {

    if (is_kind(kind, __ACK__)) {
        if (between(ackexpected, pkt.ackseqno, nextframetosend)) {

            printf("a\t\tACK arrived seq: %d seg: %d\n", pkt.ackseqno, pkt.acksegid);

            int acked = FALSE;

            // if all fragments arrived and fall into the window
            int pktackseqno = pkt.ackseqno % NRBUFS;
            while (between(ackexpected, pkt.ackseqno, nextframetosend)) {

                // avoid recomputing the modulus
                int ackexpected_mod = ackexpected % NRBUFS;

                // check that all of the fragments are acknowledged
                int i;
                if (ackexpected != pkt.ackseqno) {
                    for (i = 0; i < numacks[ackexpected_mod]; i++) {
                        arrivedacks[ackexpected_mod][i] = TRUE;
                    }
                    allackssarrived[ackexpected_mod] = TRUE;
                } else {
                    for (i = 0; i <= pkt.acksegid; i++) {
                        arrivedacks[pktackseqno][i] = TRUE;
                    }
                    allackssarrived[pktackseqno] = arrivedacks[pktackseqno][numacks[pktackseqno]-1];
                }

                // some fragments stayed unacknowledged
                if(allackssarrived[ackexpected_mod] == FALSE)
                    break;

                // we have acked something so enable the application layer
                acked = TRUE;

                // reset the variables for the next packet
                --nbuffered;

                for (i = 0; i < numacks[ackexpected_mod]; i++)
                    arrivedacks[ackexpected_mod][i] = FALSE;

                numacks[ackexpected_mod] = 0;

                dests[ackexpected_mod] = -1;
                allackssarrived[ackexpected_mod] = FALSE;

                inc(&ackexpected);

                if (timers[ackexpected_mod] != NULLTIMER)
                    CNET_stop_timer(timers[ackexpected_mod]);

                timers[ackexpected_mod] = NULLTIMER;
            }
            if (acked)
                CNET_enable_application(ALLNODES);
        }
    }
    if (is_kind(kind, __DATA__)) {
        printf("i\t\tDATA arrived seq: %d seg: %d bet: %d arr: %d\n", pkt.seqno, pkt.segid, between(frameexpected, pkt.seqno, toofar), arrivedfrags[pkt.seqno % NRBUFS][pkt.segid]);
        if (between(frameexpected, pkt.seqno, toofar) && arrivedfrags[pkt.seqno % NRBUFS][pkt.segid] == FALSE) {

            // avoid recomputing the modulus
            int pkt_seqno_mod = pkt.seqno % NRBUFS;

            // figure out mtu and number of fragments to come
            int lastmsg = is_kind(kind, __LASTSGM__);

            assert(lastmsg == 1 || lastmsg == 0);
            if (lastmsg == 0) {
                mtusize[pkt_seqno_mod] = length - PACKET_HEADER_SIZE;
            }
            if(lastmsg == 1) {
                numfrags[pkt_seqno_mod] = pkt.segid + 1;
                if (pkt.segid == 0) {
                    mtusize[pkt_seqno_mod] = length - PACKET_HEADER_SIZE;
                }
            }

            // mark fragment arrived and copy content to sliding window
            if (mtusize[pkt_seqno_mod] != 0) {
                inpacket[pkt_seqno_mod].seqno = pkt.seqno;
                arrivedfrags[pkt_seqno_mod][pkt.segid] = TRUE;
                uint32_t offset = mtusize[pkt_seqno_mod] * pkt.segid;
                size_t mess_len = length - PACKET_HEADER_SIZE;
                memcpy((char*) inpacket[pkt_seqno_mod].msg + offset, (char*) pkt.msg, mess_len);

                inlengths[pkt_seqno_mod] += (length - PACKET_HEADER_SIZE);
            }

            // find if all fragments arrived
            if (numfrags[pkt_seqno_mod] > 0) {
                int f;
                arrived[pkt_seqno_mod] = TRUE;
                lastfrag[pkt_seqno_mod] = -1;
                for (f = 0; f < numfrags[pkt_seqno_mod]; f++) {
                    if (arrivedfrags[pkt_seqno_mod][f] == FALSE) {
                        arrived[pkt_seqno_mod] = FALSE;
                        break;
                    }
                    else {
                        lastfrag[pkt_seqno_mod] = f;
                    }
                }
            }
            // deliver arrived messages to application layer
            int arrived_frame = FALSE;
            while (arrived[frameexpected % NRBUFS]) {
                arrived_frame = TRUE;
                int frameexpected_mod = frameexpected % NRBUFS;

                size_t len = inlengths[frameexpected_mod];

                printf("^\t\tto application len: %llu seq: %d\n", len, frameexpected);

                // reset the variables for next packet
                inlengths[frameexpected_mod] = 0;
                arrived[frameexpected_mod] = FALSE;

                int f;
                for (f = 0; f < numfrags[frameexpected_mod]; f++)
                    arrivedfrags[frameexpected_mod][f] = FALSE;

                mtusize[frameexpected_mod] = 0;
                numfrags[frameexpected_mod] = 0;
                lastfullfragment = lastfrag[frameexpected_mod];
                lastfrag[frameexpected_mod] = -1;

                inc(&frameexpected);
                inc(&toofar);

                // push the message to application layer
                CHECK(CNET_write_application((char*)inpacket[frameexpected_mod].msg, &len));
            }

            // start a separate ack timer
            if(arrived_frame == TRUE) {
                if(separate_ack_timer != NULLTIMER)
                               CNET_stop_timer(separate_ack_timer);
                    separate_ack_timer = CNET_start_timer(EV_TIMER3, 100, src);
            }
        }
        else
            printf("i\t\tDATA ignored seq: %d seg: %d\n", pkt.seqno, pkt.segid);
    }
}
//-----------------------------------------------------------------------------
// timeout for a separate acknowledgement
void separate_ack_timeout(CnetEvent ev, CnetTimerID t1, CnetData data) {

    uint8_t src = (uint8_t) data;

    separate_ack_timer = NULLTIMER;

    PACKET frg;
    transmit_packet(__ACK__, src, PACKET_HEADER_SIZE, frg);
}
//-----------------------------------------------------------------------------
// called for retransmitting the timeouted packet. performs fragmentation
// of the packet and transmitting unacknowledged fragments
void ack_timeout(CnetEvent ev, CnetTimerID t1, CnetData data) {

    int seqno = (int) data;
    int seqno_mod = seqno % NRBUFS;

    printf("i\t\tDATA timeout seq: %d\n", seqno);

    timers[seqno_mod] = NULLTIMER;

    // find out where to send and how to fragment
    int link = get_next_link_for_dest(dests[seqno_mod]);
    int mtu = get_mtu_for_link(link);

    // segment id and fraction of packet transmitted
    uint32_t segid = 0;
    uint32_t total_length = 0;

    int pkt_len = outlengths[outpacket[seqno_mod].seqno % NRBUFS];

    // fragment timouted packet and transmit
    while (total_length < pkt_len) {

        PACKET frg_seg;

        size_t frg_seg_len;
        if (pkt_len - total_length < mtu)
            frg_seg_len = pkt_len - total_length;
        else
            frg_seg_len = mtu;

        // send only unacknowledged fragments
        if (arrivedacks[seqno_mod][segid] == FALSE) {
            uint8_t frg_seg_kind = __DATA__;

            if (total_length + mtu >= pkt_len)
                frg_seg_kind |= __LASTSGM__;

            memcpy((char*)frg_seg.msg, (char*)outpacket[seqno_mod].msg + segid * mtu, frg_seg_len);
            frg_seg.seqno = outpacket[seqno_mod].seqno;
            frg_seg.segid = segid;

            transmit_packet(frg_seg_kind, dests[seqno_mod], PACKET_HEADER_SIZE+frg_seg_len, frg_seg);
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
    frg.len = sizeof(FRAGMENT);

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

    if(flush_queue_timer == NULLTIMER) {
        CnetTime timeout = 1;
        flush_queue_timer = CNET_start_timer(EV_TIMER2, timeout, (CnetData) -1);
    }

    queue_add(fragment_queue, &frg, frg_len);
}
//-----------------------------------------------------------------------------
void print_timers() {
    printf("\t\ttimers ");
    int i, j;
    for(i = 0; i < NRBUFS; i++)
        printf("%d ", timers[i]);
    printf("\n");

    printf("\t\tarrivedacks\n");
    for(i = 0; i < NRBUFS; i++) {
        printf("\t\t\tanum: %d numacks: %d %d (", outpacket[i].seqno, numacks[i], allackssarrived[i]);
        for(j = 0; j < MAXFR; j++)
            printf(" %d", arrivedacks[i][j]);
        printf(")\n");
    }
    printf("\n");

    printf("\t\tarrivedfragss\n");
    for(i = 0; i < NRBUFS; i++) {
        printf("\t\t\tfnum: %d numfrags: %d lastfrg: %d %d (", inpacket[i].seqno, numfrags[i], lastfrag[i], arrived[i]);
        for(j = 0; j < MAXFR; j++)
            printf(" %d", arrivedfrags[i][j]);
        printf(")\n");
    }
    printf("\n");

    printf("\t\tmtusize ");
    for(i = 0; i < NRBUFS; i++)
        printf("%d ", mtusize[i]);
    printf("\n");
}
//-----------------------------------------------------------------------------
static void show_vars(CnetEvent ev, CnetTimerID t1, CnetData data) {
    puts("\n\t--------------------");
    printf("\tframeexpected\t= %d\n", frameexpected);
    printf("\tackexpected\t= %d\n", ackexpected);
    printf("\tnextframetosend\t= %d\n", nextframetosend);
    printf("\tlastfullfragment\t= %d\n", lastfullfragment);
    printf("\tnbuffered\t= %d\n", nbuffered);
    printf("\ttransport queue size\t = %d\n", queue_nitems(fragment_queue));
    print_network_layer();
    print_timers();
    puts("\t--------------------\n");
}
//-----------------------------------------------------------------------------
// clean all allocated memory
static void shutdown(CnetEvent ev, CnetTimerID t1, CnetData data) {
    show_vars(ev, t1, data);

    queue_free(fragment_queue);

    shutdown_network();
}
//-----------------------------------------------------------------------------
// initialize transport layer variables
void init_transport() {
    fragment_queue = queue_new();

    int b;
    for (b = 0; b < NRBUFS; b++) {
        arrived[b] = FALSE;
        timers[b] = NULLTIMER;
        dests[b] = -1;

        allackssarrived[b] = 0;
        numfrags[b] = -1;
        numacks[b] = 0;
        mtusize[b] = 0;
        lastfrag[b] = -1;

        int f;
        for (f = 0; f < MAXFR; f++)
            arrivedacks[b][f] = FALSE;

        for (f = 0; f < MAXFR; f++)
            arrivedfrags[b][f] = FALSE;
    }

    CHECK(CNET_set_handler( EV_DEBUG1, show_vars, 0));

    CHECK(CNET_set_handler( EV_TIMER1, ack_timeout, 0));
    CHECK(CNET_set_handler( EV_TIMER2, flush_queue, 0));
    CHECK(CNET_set_handler( EV_TIMER3, separate_ack_timeout, 0));

    CHECK(CNET_set_handler( EV_SHUTDOWN, shutdown, 0));
    CHECK(CNET_set_debug_string(EV_DEBUG1, "Show Vars"));

    CHECK(CNET_set_handler( EV_APPLICATIONREADY, write_transport, 0));

    flush_queue_timer = NULLTIMER;
    separate_ack_timer = NULLTIMER;
}
//-----------------------------------------------------------------------------

