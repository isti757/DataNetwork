/*
 * network_layer.c
 *
 *  Created on: Jul 30, 2011
 *      Author: isti
 */
#include <cnet.h>
#include <stdlib.h>
#include "network_layer.h"

//-----------------------------------------------------------------------------
//initialize the network table
void init_network() {
    ;
}
//-----------------------------------------------------------------------------
// read an outgoing packet into network layer
void write_network(uint8_t kind, CnetAddr dest, uint16_t packet_len,
        PACKET packet) {

    int link = get_next_link_for_dest(dest);

    DATAGRAM dtg;

    size_t packet_size_t = packet_len;
    memcpy((char*) &dtg.payload, (char*) &packet, packet_size_t);
    dtg.dest = dest;
    dtg.src = nodeinfo.address;
    dtg.kind = kind;
    dtg.length = packet_len;
    //dtg.checksum = CNET_ccitt((unsigned char *) &dtg, DATAGRAM_SIZE(dtg));

    write_datalink(link, dtg);
}
//-----------------------------------------------------------------------------
// write an incoming message from datalink to transport layer
void read_network(int link, DATAGRAM dtg, int length) {
    read_transport(dtg.kind, length - DATAGRAM_HEADER_SIZE, dtg.src,
            dtg.payload);
}
//-----------------------------------------------------------------------------
// detect a link for outcoming message
int get_next_link_for_dest(CnetAddr destaddr) {
    return 1;
}
//-----------------------------------------------------------------------------
// detect fragmentation size for the specified link
int get_mtu_for_link(int link) {
    return 96 - PACKET_HEADER_SIZE - DATAGRAM_HEADER_SIZE;
}
//-----------------------------------------------------------------------------
void shutdown_network() {
    shutdown_datalink();
}
//-----------------------------------------------------------------------------
void print_network_layer() {
    print_datalink_layer();
}
//-----------------------------------------------------------------------------
