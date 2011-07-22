/*
 * datalink_layer.h
 *
 *  Created on: Jun 28, 2011
 *      Author: isti
 */

#ifndef DATALINK_LAYER_H_
#define DATALINK_LAYER_H_

#include "frame.h"
#include "sliding_window.h"

/**
 * Provides :
 * 1. adaptive timeout functionality
 * 2. sliding window (for both sender and reciever)
 * 3. possibly piggy backing and negative acknowledgements
 */

/*
static PACKET lastmsg;
static size_t lastlength = 0;
static CnetTimerID lasttimer = NULLTIMER;

static int ackexpected = 0;
static int nextframetosend = 0;
static int frameexpected = 0;
*/

static void transmit_frame(PACKET *msg, FRAMEKIND kind, size_t msglen, int seqno) {
	FRAME f;

	// TODO: put into a constructor
	f.kind = kind;
	f.seq = seqno;
	f.checksum = 0;
	f.len = msglen;

	switch (kind) {
		case DL_ACK :
			DEBUG2("ACK transmitted, seq=%d\n", seqno);
			break;

		case DL_DATA :

			;CnetTime retransmit_timeout, propagation_timeout;

			memcpy(&f.msg, (char *) msg, (int) msglen);
			DEBUG2(" DL_DATA transmitted, seq=%d\n", seqno);

			// retransmit timeout (used to be 8000000, maybe have to change back)
			// measured in microseconds, i.e. 10^-6 of a second
			// bandwidth is measured in bits per second so multiplied by 8
			retransmit_timeout = ceil((CnetTime) (float)(8 * 1000000 * FRAME_SIZE(f)) / (float)linkinfo[1].bandwidth)
					+ linkinfo[1].propagationdelay;

			// disables application layer until the next message is out of the link
			propagation_timeout = (CnetTime)ceil((float)(8 * 1000000 * FRAME_SIZE(f)) / (float)linkinfo[1].bandwidth)+
								(CnetTime)linkinfo[1].propagationdelay;

			DEBUG_NODE_STATISTICS();

			printf("frame size: %d\n", FRAME_SIZE(f));
			printf("bandwidth: %d\n", linkinfo[1].bandwidth);
			printf("transmission delay : %d\n",  retransmit_timeout);
			printf("propagation delay : %d\n",  propagation_timeout);

			CNET_start_timer(EV_TIMER2, propagation_timeout, nodeinfo.address);

			CNET_disable_application(ALLNODES);

			//set_frame_timer(&sender_window, nodeinfo.address, retransmit_timeout);
			//CNET_trace_name(&f.seq, "transmitting");
			break;
		case DL_NACK:
			break;
	}
	msglen = FRAME_SIZE(f);
//	f.checksum = CNET_ccitt((unsigned char *) &f, (int) msglen);

	CHECK(CNET_write_physical(1, (char *)&f, &msglen));
}

static EVENT_HANDLER(application_timeouts) {
	CNET_enable_application(ALLNODES);
}

static EVENT_HANDLER(timeouts) {
	int seqno = (int) data;
	seqno--;
	//stop_frame_timer(&sender_window, seqno);

	//DEBUG2("timeout, resending data, seq=%d\n", seqno);
	//transmit_frame(get_message(&sender_window, seqno), DL_DATA, PACKET_SIZE((*get_message(&sender_window, seqno))), seqno);
}

static EVENT_HANDLER(physical_ready) {
	DEBUG1("recieving on physical layer\n");
	FRAME f;
	size_t len;
	int link; //, checksum;
	len = sizeof(FRAME);
	CHECK(CNET_read_physical(&link, (char *)&f, &len));
//	checksum = f.checksum;
//	f.checksum = 0;
//	if (CNET_ccitt((unsigned char *) &f, (int) len) != checksum) {
//		DEBUG1("\t\t\t\tBAD checksum - frame ignored\n");
//		return; // bad checksum, ignore frame
//	}

	switch (f.kind) {
		case DL_ACK:
			DEBUG2("ACK received, seq=%d,\n", f.seq);
			print_snd(&sender_window);
			int insideSender=inside_current_window_sender(&sender_window, f.seq);
			//printf("Is inside window=%d\n",insideSender);
			if (insideSender) {
				while(inside_current_window_sender(&sender_window,f.seq))
				{
					decrease_buffered_size(&sender_window);
					//DEBUG1("ACK received, mark in loop");

					//stop_frame_timer(&sender_window, f.seq);
					increment_first_message_number(&sender_window);
				}
				// TODO: used to be ALLNODES
				CNET_enable_application(ALLNODES);
				//DEBUG1("Enabling application\n");
				print_snd(&sender_window);
			} else {
				//DEBUG1("Not inside reciever window\n");
				print_snd(&sender_window);
			}

			break;
		case DL_DATA:
			DEBUG2("\t\tDATA received, seq=%d,\n", f.seq);
			int inside=inside_current_window_receiver(&receiver_window, f.seq);
			//printf("Is inside window=%d\n",inside);
			int hasArrived=has_arrived(&receiver_window, f.seq);
			//printf("Is arrived?=%d\n",hasArrived);
			print_rcv(&receiver_window);
			if (inside && (hasArrived == 0)) {
				len = f.len;
				put_into_received(&receiver_window, f.seq, f.msg);

				while(has_arrived(&receiver_window, receiver_window.first_frame_expected))
				{
					DEBUG2("up to application %d \n", receiver_window.first_frame_expected);
					len = frame_expected_length(&receiver_window, receiver_window.first_frame_expected);
					CHECK(CNET_write_application((char *)get_expected_frame_data(&receiver_window), &len));
					unmark_arrived(&receiver_window, receiver_window.first_frame_expected);

				}
				print_rcv(&receiver_window);
			} else {
				DEBUG2("ignored frame %d\n",f.seq);
				print_rcv(&receiver_window);
			}
			//frameexpected + MAXSEQ) % (MAXSEQ+1)
			int acknowledgement_id = (receiver_window.first_frame_expected+BUFFER_SIZE-1)%BUFFER_SIZE;
			DEBUG2("Sending ACK, id=%d",acknowledgement_id);
			transmit_frame((PACKET *) NULL, DL_ACK, 0, acknowledgement_id);
			break;
		case DL_NACK:
			break;
	}
}

#endif /* DATALINK_LAYER_H_ */
