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

	f.kind = kind;
	f.seq = seqno;
	f.checksum = 0;
	f.len = msglen;

	switch (kind) {
	case DL_ACK :

		printf("ACK transmitted, seq=%d\n", seqno);
		break;

	case DL_DATA :

		;CnetTime timeout;

		memcpy(&f.msg, (char *) msg, (int) msglen);
		printf(" DL_DATA transmitted, seq=%d\n", seqno);

		timeout = FRAME_SIZE(f) * ((CnetTime) 8000000 / linkinfo[1].bandwidth)
				+ linkinfo[1].propagationdelay;

		set_frame_timer(&sender_window, f.seq, timeout);
		CNET_trace_name(&f.seq, "transmitting");
		break;
	case DL_NACK:
		break;
	}
	msglen = FRAME_SIZE(f);
	f.checksum = CNET_ccitt((unsigned char *) &f, (int) msglen);
	CHECK(CNET_write_physical(1, (char *)&f, &msglen));
}

/*static EVENT_HANDLER(timeouts) {
	int seqno = (int) data;

	stop_frame_timer(&sender_window, seqno);

	printf("timeout, resending data, seq=%d\n", seqno);
	transmit_frame(get_message(&sender_window, seqno), DL_DATA, PACKET_SIZE((*get_message(&sender_window, seqno))), seqno);
}*/

static EVENT_HANDLER(physical_ready) {
	printf("recieving on physical layer, seq=%d\n", 0);
	FRAME f;
	size_t len;
	int link, checksum;

	len = sizeof(FRAME);
	CHECK(CNET_read_physical(&link, (char *)&f, &len));

	checksum = f.checksum;
	f.checksum = 0;
	if (CNET_ccitt((unsigned char *) &f, (int) len) != checksum) {
		printf("\t\t\t\tBAD checksum - frame ignored\n");
		return; // bad checksum, ignore frame
	}

	switch (f.kind) {
	case DL_ACK:
		if (inside_current_window_sender(&sender_window,f.seq)) {
			while(inside_current_window_sender(&sender_window,f.seq))
			{
				decrease_buffered_size(&sender_window);
				printf("\t\t\t\tACK received, seq=%d\n", f.seq);

				stop_frame_timer(&sender_window, f.seq);
				increment_first_message_number(&sender_window);
			}
			CNET_enable_application(ALLNODES);
		}
		break;
	case DL_DATA:
		printf("\t\tDATA received, seq=%d, ", f.seq);
		if (inside_current_window_receiver(&receiver_window, f.seq) && has_arrived(&receiver_window, f.seq) == 0) {
			len = f.len;
			put_into_received(&receiver_window, f.seq, f.msg);

			while(has_arrived(&receiver_window, f.seq))
			{
				printf("up to application %d \n", receiver_window.first_frame_expected);
				len = frame_expected_length(&receiver_window, f.seq);
				CHECK(CNET_write_application(f.msg, &len));

				unmark_arrived(&receiver_window, receiver_window.first_frame_expected);
			}
		} else
			printf("ignored\n");

		//int acknowledgement_id = (receiver_window.first_frame_expected+receiver_window.buffer_size-1)%receiver_window.buffer_size;
		//transmit_frame((PACKET *) NULL, DL_ACK, 0, acknowledgement_id);
		break;
	case DL_NACK:
		break;
	}
}

#endif /* DATALINK_LAYER_H_ */
