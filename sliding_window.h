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

const static int SLIDING_WINDOW_SIZE = 10;

typedef struct
{
	// packets for sending
	PACKET      *out_packets;
	// arrived status
	bool        *arrived;
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
	ssw->arrived = malloc(SLIDING_WINDOW_SIZE * sizeof(bool));
	ssw->timer = malloc(SLIDING_WINDOW_SIZE * sizeof(CnetTimerID));

	for(int i = 0; i < SLIDING_WINDOW_SIZE; i++)
	{
		ssw->arrived[i] = false;
		ssw->timer[i] = NULLTIMER;
	}
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

char* get_next_message_data(const sender_sliding_window *ssw)
{
	return get_next_message(ssw)->data;
}

void increment_next_message_number(sender_sliding_window *ssw)
{
	ssw->next_frame = (ssw->next_frame+1)%SLIDING_WINDOW_SIZE;
}

void set_message_size(sender_sliding_window *ssw, int msg_id, int size)
{
	ssw->out_packets[msg_id].len = size;
}

void set_frame_timer(sender_sliding_window *ssw, int msg_id, CnetTime timeout)
{
	ssw->timer[msg_id%SLIDING_WINDOW_SIZE] = CNET_start_timer(EV_TIMER1, timeout, 0);
}

void stop_frame_timer(sender_sliding_window *ssw, int msg_id)
{
	CNET_stop_timer(ssw->timer[msg_id%SLIDING_WINDOW_SIZE]);
	ssw->timer[msg_id%SLIDING_WINDOW_SIZE] = NULLTIMER;
}

int inside_current_window(sender_sliding_window *ssw, int msg_id)
{

}

typedef struct
{
	int id;
} reciever_sliding_window;

#endif /* SLIDING_WINDOW_H_ */
