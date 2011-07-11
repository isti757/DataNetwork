/*
 * milestone1.c
 *
 *  Created on: Jun 24, 2011
 *      Author: isti
 */


#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "frame.h"
#include "datalink_layer.h"
#include "network_layer.h"
#include "application_layer.h"



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
    case DL_NACK:
    	break;
    }
}

static EVENT_HANDLER(showstate) {
	printf( "%0*.*f asdfsadf", 8, 4, 2.5 );

}

EVENT_HANDLER(reboot_node) {
	if (nodeinfo.nodenumber > 1) {
		fprintf(stderr, "This is not a 2-node network!\n");
		exit(1);
	}

	init_receiver_window(&receiver_window);
	init_sender_sliding_window(&sender_window);

	CHECK(CNET_set_handler( EV_APPLICATIONREADY, application_ready, 0));
	CHECK(CNET_set_handler( EV_PHYSICALREADY, physical_ready, 0));
	CHECK(CNET_set_handler( EV_DRAWFRAME, draw_frame, 0));
	CHECK(CNET_set_handler( EV_TIMER1, timeouts, 0));
	CHECK(CNET_set_handler( EV_DEBUG1, showstate, 0));

	CHECK(CNET_set_debug_string( EV_DEBUG1, "State"));

	if (nodeinfo.nodenumber == 1)
		CNET_enable_application(ALLNODES);


}

