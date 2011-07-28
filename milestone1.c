/*
 * milestone1.c
 *
 *  Created on: Jul 18, 2011
 *      Author: Igor, Kyrill, Sanjar (Group 7)
 */

#include <cnet.h>
#include <stdlib.h>
#include <string.h>

#include <sys/queue.h>

//-----------------------------------------------------------------------------
// Message data structure
//
typedef struct {
	char data[MAX_MESSAGE_SIZE];
} MSG;
//-----------------------------------------------------------------------------
// The frame data structure for transmitting over the link. Carries a message and
// the length of message carries
//
typedef struct {
	size_t len; // the length of the msg field only
	MSG msg;
} FRAME;

#define FRAME_HEADER_SIZE  (sizeof(FRAME) - sizeof(MSG))
#define FRAME_SIZE(f)      (FRAME_HEADER_SIZE + f.len)

// last message passed from application layer
static MSG *lastmsg;
// the length of the lastmsg
static size_t lastlength = 0;
// the timer used for waiting until the message is on the link
static CnetTimerID lasttimer = NULLTIMER;
// variables for debugging information
static int lastframereceived = 0;
static int nextframetosend = 0;
//-----------------------------------------------------------------------------
// Frame queue data structure, might be useful when application layer generates
// messages faster than sending the frame. It accumulates frames so far and turns
// off application layer until all of the messages in the queue have been
// transmited
//
struct frame_queue_t {
	FRAME frame;
	LIST_ENTRY(frame_queue_t) frames;
};

LIST_HEAD(frames_head, frame_queue_t) frame_queue;
//-----------------------------------------------------------------------------
// Transmits the message of given length and seq number and sets the timer.
// When timer expires it gets called again on the next message in the queue
//
static void transmit_frame(MSG *msg, size_t length, int seqno) {

	CNET_disable_application(ALLNODES);

	FRAME f;
	f.len = length;

	int link = 1;

	printf(" DATA transmitted, seq=%d\n", seqno);
	memcpy(&f.msg, (char *) msg, (int) length);

	// Set timer to propagation delay+transmission delay to ensure the
	// frame is out of the link when the next one is sent
	CnetTime timeout;
	timeout = ((CnetTime) (FRAME_SIZE(f) * 8000000) / linkinfo[link].bandwidth)
			+ linkinfo[link].propagationdelay;

	printf("timeout: %llu\n", timeout);

	lasttimer = CNET_start_timer(EV_TIMER1, timeout, 0);

	length = FRAME_SIZE(f);

	CHECK(CNET_write_physical(link, (char *)&f, &length));
}
//-----------------------------------------------------------------------------
static EVENT_HANDLER(application_ready) {
	// read the message from application layer
	CnetAddr destaddr;

	lastlength = sizeof(MSG);
	CHECK(CNET_read_application(&destaddr, (char *)lastmsg, &lastlength));
	CNET_disable_application(ALLNODES);

	printf("down from application, seq=%d\n", nextframetosend);

	// set the timer to handle the current message
	CnetTime timeout = 100;
	CNET_start_timer(EV_TIMER1, timeout, 0);

	nextframetosend++;

	// insert the frame into the queue
	FRAME frame;
	frame.msg = *lastmsg;
	frame.len = lastlength;

	struct frame_queue_t *cur_frame = malloc(sizeof(struct frame_queue_t));

	cur_frame->frame = frame;

	LIST_INSERT_HEAD(&frame_queue, cur_frame, frames);

	// debugging information
	int queue_size = 0;
	LIST_FOREACH(cur_frame, &frame_queue, frames) {
		printf("  queue: %d\n", cur_frame->frame.len);
		queue_size++;
	}

	printf("  queue size: %d\n\n", queue_size);
}
//-----------------------------------------------------------------------------
// Writes whatever it receives up to the application layer
//
static EVENT_HANDLER(physical_ready) {
	FRAME f;
	size_t len;
	int link;

	len = sizeof(FRAME);
	CHECK(CNET_read_physical(&link, (char *)&f, &len));

	printf("DATA received, seq=%d\n", lastframereceived);
	lastframereceived++;

	len = f.len;
	CHECK(CNET_write_application((char *)&f.msg, &len));
}
//-----------------------------------------------------------------------------
// Is called when a new message in the queue can be sent
//
static EVENT_HANDLER(timeouts) {
	CNET_enable_application(ALLNODES);
	lasttimer = NULLTIMER;

	// nothing to send
	if (LIST_EMPTY(&frame_queue))
		return;

	struct frame_queue_t *cur_frame;

	// send only one message waiting in the queue
	LIST_FOREACH(cur_frame, &frame_queue, frames) {
		transmit_frame(&cur_frame->frame.msg, cur_frame->frame.len,
				nextframetosend);
		LIST_REMOVE(cur_frame, frames);
		free(cur_frame);
		break;
	}
}
//-----------------------------------------------------------------------------
// Clean whatever is left in the queue and free the memory
//
static EVENT_HANDLER(shutdown) {
	if (LIST_EMPTY(&frame_queue))
			return;

	struct frame_queue_t *cur_frame;
	LIST_FOREACH(cur_frame, &frame_queue, frames) {
		LIST_REMOVE(cur_frame, frames);
		free(cur_frame);
	}

	free(lastmsg);
}
//-----------------------------------------------------------------------------
static EVENT_HANDLER(showstate) {
	;
}
//-----------------------------------------------------------------------------
EVENT_HANDLER(reboot_node) {
	if (nodeinfo.nodenumber > 1) {
		fprintf(stderr, "This is not a 2-node network!\n");
		exit(1);
	}

	LIST_INIT(&frame_queue);

	lastmsg = calloc(1, sizeof(MSG));

	CHECK(CNET_set_handler( EV_APPLICATIONREADY, application_ready, 0));
	CHECK(CNET_set_handler( EV_PHYSICALREADY, physical_ready, 0));
	CHECK(CNET_set_handler( EV_TIMER1, timeouts, 0));
	CHECK(CNET_set_handler( EV_DEBUG0, showstate, 0));
	CHECK(CNET_set_handler( EV_SHUTDOWN, shutdown, 0));

	CHECK(CNET_set_debug_string( EV_DEBUG0, "State"));

	//if(nodeinfo.nodenumber == 0)
	CNET_enable_application(ALLNODES);
}
