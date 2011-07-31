/*
 * datalink_layer.c
 *
 *  Created on: Jul 30, 2011
 *      Author: isti
 */

#include "datalink_layer.h"

//-----------------------------------------------------------------------------
// write an incoming frame into datalink layer
void read_datalink(int link, DATAGRAM dtg)
{
	// Set timer to propagation delay+transmission delay to ensure the
	// frame is out of the link when the next one is sent
//  CnetTime timeout;
//	timeout = ((CnetTime) (FRAME_SIZE(f) * 8000000) / linkinfo[link].bandwidth)
//			+ linkinfo[link].propagationdelay;

	int dtg_size = DATAGRAM_SIZE(dtg);
	CHECK(CNET_write_physical(link, (char *)&dtg, (size_t*)&dtg_size));
}
//-----------------------------------------------------------------------------

