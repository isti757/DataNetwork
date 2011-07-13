/*
 * application_layer.h
 *
 *  Created on: Jun 28, 2011
 *      Author: isti
 */

#ifndef APPLICATION_LAYER_H_
#define APPLICATION_LAYER_H_

#include "frame.h"
#include "sliding_window.h"

static EVENT_HANDLER(application_ready) {
	CnetAddr destaddr;

	int next_frame = get_next_message_number(&sender_window);
	int next_frame_length = MAX_MESSAGE_SIZE;
	prepare_next_message(&sender_window);

	CHECK(CNET_read_application(&destaddr, get_next_message_data(&sender_window), (size_t*)&next_frame_length));
	printf("down from application, seq=%d\n", next_frame);

	if(increase_buffered_size(&sender_window)==0) {
		CNET_disable_application(ALLNODES);
		printf("Disabling application\n");
	}
	set_message_size(&sender_window, next_frame, next_frame_length);
	//get_next_message(&sender_window)->len=next_frame_length;
	transmit_frame(get_next_message(&sender_window), DL_DATA, PACKET_SIZE(get_next_message(&sender_window)),
			sender_window.next_frame);

	increment_next_message_number(&sender_window);
	print_snd(&sender_window);
}

#endif /* APPLICATION_LAYER_H_ */
