/*
 * milestone1.c
 *
 *  Created on: Jun 24, 2011
 *      Author: isti
 */

#include <cnet.h>

#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
	DL_DATA, DL_ACK
} FRAMEKIND;

typedef struct {
	char data[MAX_MESSAGE_SIZE];
} MSG;

typedef struct {
	FRAMEKIND kind; // only ever DL_DATA or DL_ACK
	size_t len; // the length of the msg field only
	int checksum; // checksum of the whole frame
	int seq; // only ever 0 or 1
	MSG msg;
} FRAME;

#define FRAME_HEADER_SIZE  (sizeof(FRAME) - sizeof(MSG))
#define FRAME_SIZE(f)      (FRAME_HEADER_SIZE + f.len)

static MSG lastmsg;
static size_t lastlength = 0;
static CnetTimerID lasttimer = NULLTIMER;

static int ackexpected = 0;
static int nextframetosend = 0;
static int frameexpected = 0;

static EVENT_HANDLER(showstate) {
	printf(
			"\tackexpected\t= %d\n\tnextframetosend\t= %d\n\tframeexpected\t= %d\n",
			ackexpected, nextframetosend, frameexpected);

}

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

static EVENT_HANDLER(application_ready) {
	CnetAddr destaddr;

	lastlength = sizeof(MSG);
	CHECK(CNET_read_application(&destaddr, (char *)&lastmsg, &lastlength));
	CNET_disable_application(ALLNODES);

	printf("down from application, seq=%d\n", nextframetosend);
	transmit_frame(&lastmsg, DL_DATA, lastlength, nextframetosend);
	nextframetosend = 1 - nextframetosend;
}

static EVENT_HANDLER(timeouts) {
	if (timer == lasttimer) {
		printf("timeout, seq=%d\n", ackexpected);
		transmit_frame(&lastmsg, DL_DATA, lastlength, ackexpected);
	}
}

void draw_frame(CnetEvent ev, CnetTimerID timer, CnetData data)
{
    CnetDrawFrame *df  = (CnetDrawFrame *)data;
    FRAME         *f   = (FRAME *)df->frame;

    switch (f->kind) {
    case DL_ACK:
        df->nfields    = 1;
        df->colours[0] = (f->seq == 0) ? "red" : "purple";
        df->pixels[0]  = 10;
        sprintf(df->text, "ack=%d", f->seq);
        break;

    case DL_DATA:
        df->nfields    = 2;
        df->colours[0] = (f->seq == 0) ? "red" : "purple";
        df->pixels[0]  = 10;
        df->colours[1] = "green";
        df->pixels[1]  = 30;
        sprintf(df->text, "data=%d", f->seq);
        break;
    }
}

EVENT_HANDLER(reboot_node) {
	if (nodeinfo.nodenumber > 1) {
		fprintf(stderr, "This is not a 2-node network!\n");
		exit(1);
	}

	CHECK(CNET_set_handler( EV_APPLICATIONREADY, application_ready, 0));
	CHECK(CNET_set_handler( EV_PHYSICALREADY, physical_ready, 0));
	CHECK(CNET_set_handler( EV_DRAWFRAME, draw_frame, 0));
	CHECK(CNET_set_handler( EV_TIMER1, timeouts, 0));
	CHECK(CNET_set_handler( EV_DEBUG1, showstate, 0));

	CHECK(CNET_set_debug_string( EV_DEBUG1, "State"));

	if (nodeinfo.nodenumber == 1)
		CNET_enable_application(ALLNODES);
}

