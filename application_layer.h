/*
 * application_layer.h
 *
 *  Created on: Aug 30, 2011
 *      Author: isti
 */
#ifndef APPLICATION_LAYER_H_
#define APPLICATION_LAYER_H_

#include <cnet.h>

//#define APPLICATION_COMPRESSION

//-----------------------------------------------------------------------------
// initialize application layer
extern void init_application();
//-----------------------------------------------------------------------------
extern void read_application(char* msg, size_t len);
//-----------------------------------------------------------------------------
// read outgoing message from application to transport layer
extern void write_application(CnetEvent ev, CnetTimerID timer, CnetData data);
//-----------------------------------------------------------------------------

#endif // APPLICATION_LAYER_H_
