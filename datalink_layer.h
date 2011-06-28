/*
 * datalink_layer.h
 *
 *  Created on: Jun 28, 2011
 *      Author: isti
 */

#ifndef DATALINK_LAYER_H_
#define DATALINK_LAYER_H_

#include "frame.h"

/**
 * Provides :
 * 1. adaptive timeout functionality
 * 2. sliding window (for both sender and reciever)
 * 3. possibly piggy backing and negative acknowledgements
 */

static MSG lastmsg;
static size_t lastlength = 0;
static CnetTimerID lasttimer = NULLTIMER;

static int ackexpected = 0;
static int nextframetosend = 0;
static int frameexpected = 0;

static void transmit_frame(MSG *msg, FRAMEKIND kind, size_t msglen, int seqno) {
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
		lasttimer = CNET_start_timer(EV_TIMER1, timeout, 0);
		break;
	}
	msglen = FRAME_SIZE(f);
	f.checksum = CNET_ccitt((unsigned char *) &f, (int) msglen);
	CHECK(CNET_write_physical(1, (char *)&f, &msglen));
}

static EVENT_HANDLER(timeouts) {
	if (timer == lasttimer) {
		printf("timeout, seq=%d\n", ackexpected);
		transmit_frame(&lastmsg, DL_DATA, lastlength, ackexpected);
	}
}

static EVENT_HANDLER(physical_ready) {
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
		if (f.seq == ackexpected) {
			printf("\t\t\t\tACK received, seq=%d\n", f.seq);
			CNET_stop_timer(lasttimer);
			ackexpected = 1 - ackexpected;
			CNET_enable_application(ALLNODES);
		}
		break;
	case DL_DATA:
		printf("\t\t\t\tDATA received, seq=%d, ", f.seq);
		if (f.seq == frameexpected) {
			printf("up to application\n");
			len = f.len;
			CHECK(CNET_write_application((char *)&f.msg, &len));
			frameexpected = 1 - frameexpected;
		} else
			printf("ignored\n");
		transmit_frame((MSG *) NULL, DL_ACK, 0, f.seq);
		break;

	}
}

#endif /* DATALINK_LAYER_H_ */
