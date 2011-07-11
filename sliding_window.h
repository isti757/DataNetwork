/*
 * sliding_window.h
 *
 *  Created on: Jun 28, 2011
 *      Author: isti
 */

#ifndef SLIDING_WINDOW_H_
#define SLIDING_WINDOW_H_

#include "frame.h"
#include "packet.h"

const static int SLIDING_WINDOW_SIZE = 3;
const static int BUFFER_SIZE = 6;

typedef struct
{
	// packets for sending
	PACKET      *out_packets;

	// timers for outgoing frames
	CnetTimerID *timer;

	// number of slots taken (from beginning)
	int nbuffered;

	// beginning of sender's window sequence number
	int first_frame;
	// next sequence number (if window is shifted)
	int next_frame;

} sender_sliding_window;

static sender_sliding_window sender_window;

void decrease_buffered_size(sender_sliding_window *ssw)
{
	if(ssw->nbuffered > 0)
		ssw->nbuffered--;
	else
		abort();
}

int increase_buffered_size(sender_sliding_window *ssw)
{
	if(++ssw->nbuffered == SLIDING_WINDOW_SIZE)
		return 0;

	return 1;
}

int prepare_next_message(sender_sliding_window *ssw)
{
	ssw->out_packets[ssw->next_frame%SLIDING_WINDOW_SIZE].len = MAX_MESSAGE_SIZE;
	return increase_buffered_size(ssw);
}

void init_sender_sliding_window(sender_sliding_window *ssw)
{
	ssw->out_packets = malloc(SLIDING_WINDOW_SIZE * sizeof(PACKET));
	//ssw->arrived = malloc(SLIDING_WINDOW_SIZE * sizeof(bool));
	ssw->timer = malloc(SLIDING_WINDOW_SIZE * sizeof(CnetTimerID));

	for(int i = 0; i < SLIDING_WINDOW_SIZE; i++)
	{
		//ssw->arrived[i] = false;
		ssw->timer[i] = NULLTIMER;
	}
	ssw->next_frame=0;
	ssw->first_frame=0;
	ssw->nbuffered=0;
}

int get_next_message_number(const sender_sliding_window *ssw)
{
	return ssw->next_frame%SLIDING_WINDOW_SIZE;
}

int get_next_message_length(const sender_sliding_window *ssw)
{
	int next_frame = get_next_message_number(ssw);
	return ssw->out_packets[next_frame].len;
}

PACKET* get_next_message(const sender_sliding_window *ssw)
{
	int next_frame = get_next_message_number(ssw);
	return &ssw->out_packets[next_frame];
}

PACKET* get_message(const sender_sliding_window *ssw, int msg_id)
{
	return &ssw->out_packets[msg_id%SLIDING_WINDOW_SIZE];
}

char* get_next_message_data(const sender_sliding_window *ssw)
{
	return get_next_message(ssw)->data;
}

char* get_message_data(const sender_sliding_window *ssw, int msg_id)
{
	return ssw->out_packets[msg_id%SLIDING_WINDOW_SIZE].data;
}

void increment_next_message_number(sender_sliding_window *ssw)
{
	ssw->next_frame = (ssw->next_frame+1)%SLIDING_WINDOW_SIZE;
}

void increment_first_message_number(sender_sliding_window *ssw)
{
	ssw->first_frame = (ssw->first_frame+1)%SLIDING_WINDOW_SIZE;
}

void set_message_size(sender_sliding_window *ssw, int msg_id, int size)
{
	ssw->out_packets[msg_id].len = size;
}

void set_frame_timer(sender_sliding_window *ssw, int msg_id, CnetTime timeout)
{
	ssw->timer[msg_id%SLIDING_WINDOW_SIZE] = CNET_start_timer(EV_TIMER1, timeout, msg_id);
}

void stop_frame_timer(sender_sliding_window *ssw, int msg_id)
{
	CNET_stop_timer(ssw->timer[msg_id%SLIDING_WINDOW_SIZE]);
	ssw->timer[msg_id%SLIDING_WINDOW_SIZE] = NULLTIMER;
}

int inside_current_window_sender(sender_sliding_window *ssw, int msg_id)
{
	int a = ssw->first_frame;
	int b = msg_id;
	int c = ssw->next_frame;

	int between1 = (a <= b && b < c);
	int between2 = (c < a && a <= b);
	int between3 = (b < c && c < a);

	return between1 || between2 || between3;
}

typedef struct
{
	// packets for sending
	PACKET      *in_packets;
	// arrived status
	int        *arrived;
	// timers for incoming frames
	CnetTimerID *timer;

	// beginning of sender's window sequence number
	int first_frame_expected;
	// next sequence number (if window is shifted)
	int buffer_end;

	int buffer_size;
} receiver_sliding_window;

static receiver_sliding_window receiver_window;

void init_receiver_window(receiver_sliding_window *rw)
{
	rw->buffer_end = SLIDING_WINDOW_SIZE;
	rw->buffer_size = BUFFER_SIZE;
	rw->in_packets = malloc(SLIDING_WINDOW_SIZE * sizeof(PACKET));
	rw->arrived = (int *)malloc(SLIDING_WINDOW_SIZE * sizeof(int));
	for(int b=0 ; b<SLIDING_WINDOW_SIZE ; b++) {
		rw->arrived[b] = 0;
	}
}

int inside_current_window_receiver(receiver_sliding_window *rw, int msg_id)
{
	int a = rw->first_frame_expected;
	int b = msg_id;
	int c = rw->buffer_end;

	int between1 = ((a <= b) && (b < c));
	int between2 = ((c < a) && (a <= b));
	int between3 = ((b < c) && (c < a));

	return between1 || between2 || between3;
}

int has_arrived(receiver_sliding_window *rw, int msg_id)
{
	return rw->arrived[msg_id%SLIDING_WINDOW_SIZE];
}

void mark_arrived(receiver_sliding_window *rw, int msg_id)
{
	rw->arrived[msg_id%SLIDING_WINDOW_SIZE] = 1;
}

void unmark_arrived(receiver_sliding_window *rw, int msg_id)
{
	rw->arrived[msg_id%SLIDING_WINDOW_SIZE] = 0;
	rw->first_frame_expected = (rw->first_frame_expected+1)%SLIDING_WINDOW_SIZE;
	rw->buffer_end = (rw->buffer_end+1)%SLIDING_WINDOW_SIZE;
}

void put_into_received(receiver_sliding_window *rw, int msg_id, PACKET pkt)
{
	mark_arrived(rw, msg_id);
	rw->in_packets[msg_id%SLIDING_WINDOW_SIZE] = pkt;
}

int frame_expected_length(receiver_sliding_window *rw, int msg_id)
{
	return rw->in_packets[msg_id%SLIDING_WINDOW_SIZE].len;
}

char* get_expected_frame_data(receiver_sliding_window *rw)
{
	return rw->in_packets[rw->first_frame_expected%SLIDING_WINDOW_SIZE].data;
}

#endif /* SLIDING_WINDOW_H_ */
