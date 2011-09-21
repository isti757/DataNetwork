/*
 * application_layer.c
 *
 *  Created on: Aug 30, 2011
 *      Author: isti
 */

#include <cnet.h>

#include "transport_layer.h"
#include "application_layer.h"

#ifdef APPLICATION_COMPRESSION
#include "compression.h"
//-----------------------------------------------------------------------------
// compression of payload
static void init_compression() {
    in = malloc(sizeof(unsigned char)*IN_LEN);
    // init compression
    if (lzo_init() != LZO_E_OK) {
        fprintf(stderr, "internal error - lzo_init() failed !!!\n");
        abort();
    }
}
//-----------------------------------------------------------------------------
// free compression variables
static void destroy_compression() {
    free(in);
}
#endif
//-----------------------------------------------------------------------------
void read_application(char* msg, size_t len) {
#ifdef APPLICATION_COMPRESSION
    // uncompress the message
    lzo_uint new_len = len, out_len = len;
    int r = lzo1x_decompress((unsigned char*)msg, out_len, in, &new_len, NULL);
    if (r != LZO_E_OK) {
        // this should NEVER happen
        fprintf(stderr,"internal error - decompression failed: %d\n", r);
        abort();
    }
    size_t nlen = new_len;
    CHECK(CNET_write_application((char*)in, &nlen));
#else
    CHECK(CNET_write_application((char*)msg, &len));
#endif
}
//-----------------------------------------------------------------------------
void write_application(CnetEvent ev, CnetTimerID timer, CnetData data) {
    // read the message
    size_t len = MAX_MESSAGE_SIZE;
    CnetAddr destaddr;
    char msg[MAX_MESSAGE_SIZE];
    CHECK(CNET_read_application(&destaddr, (char*)msg, &len));

    // perform compression
#ifdef APPLICATION_COMPRESSION
    lzo_uint in_len = len, out_len;
    int r = lzo1x_1_compress((unsigned char *)msg, in_len, in, &out_len,wrkmem);
    if (r != LZO_E_OK) {
        fprintf(stderr,"internal error - compression failed: %d\n", r);
        abort();
    }
    // write to transport layer
    write_transport(destaddr, (char*)in, out_len);
#else
    // write to transport layer
    write_transport(destaddr, msg, len);
#endif
}
//-----------------------------------------------------------------------------
// clean all allocated memory
static void shutdown(CnetEvent ev, CnetTimerID t1, CnetData data) {
#ifdef APPLICATION_COMPRESSION
    destroy_compression();
#endif
    shutdown_transport();
}
//-----------------------------------------------------------------------------
void init_application() {
#ifdef APPLICATION_COMPRESSION
    init_compression();
#endif
    CHECK(CNET_set_handler( EV_SHUTDOWN, shutdown, 0));
    CHECK(CNET_set_handler( EV_APPLICATIONREADY, write_application, 0));
}
//-----------------------------------------------------------------------------
