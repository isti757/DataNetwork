#ifndef DATALINK_LAYER_H_
#define DATALINK_LAYER_H_

#include "datagram.h"
#include "dl_frame.h"
#include "datalink_container.h"
#include "network_layer.h"

//-----------------------------------------------------------------------------
//A timer event for Cnet_write_physical
#define EV_DATALINK_FLUSH EV_TIMER6
//-----------------------------------------------------------------------------
// initialize datalink layer
extern void init_datalink();
//-----------------------------------------------------------------------------
// read an incoming frame into datalink layer
extern void read_datalink(CnetEvent event, CnetTimerID timer, CnetData data);
//-----------------------------------------------------------------------------
// write an outgoing frame into datalink layer
extern void write_datalink(int link, char * data, uint32_t checksum, uint32_t length);
//-----------------------------------------------------------------------------
// clean the memory
extern void shutdown_datalink();
//-----------------------------------------------------------------------------
#endif /* DATALINK_LAYER_H_ */
