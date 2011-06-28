/*
 * application_layer.h
 *
 *  Created on: Jun 28, 2011
 *      Author: isti
 */

#ifndef APPLICATION_LAYER_H_
#define APPLICATION_LAYER_H_

#include "frame.h"

static EVENT_HANDLER(application_ready) {
	CnetAddr destaddr;

	lastlength = sizeof(MSG);
	CHECK(CNET_read_application(&destaddr, (char *)&lastmsg, &lastlength));
	CNET_disable_application(ALLNODES);

	printf("down from application, seq=%d\n", nextframetosend);
	transmit_frame(&lastmsg, DL_DATA, lastlength, nextframetosend);
	nextframetosend = 1 - nextframetosend;
}

#endif /* APPLICATION_LAYER_H_ */
