/*
 * transport_layer.h
 *
 *  Created on: Aug 30, 2011
 *      Author: kirill
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <cnet.h>

#include "packet.h"
#include "debug.h"
#include "network_layer.h"
#include "datalink_layer.h"

//-----------------------------------------------------------------------------
// initialize the network table and run discovery
void init_network() {
    init_routing();
    init_discovery();
}
//-----------------------------------------------------------------------------
static unsigned int packets_forwarded_total = 0;
void histogram() {
    ;//fprintf(stderr, "\tforwarded: %u\n", packets_forwarded_total);
}
//-----------------------------------------------------------------------------
// write an outcoming packet into network layer
void write_network(uint8_t kind, CnetAddr address,uint16_t length, char* packet) {
    DATAGRAM dtg = alloc_datagram((kind | __TRANSPORT__), nodeinfo.address,
                                  address, packet,length);

    //copy the payload
    size_t packet_length = length;
    memcpy(&(dtg.payload), packet, packet_length);
    route(dtg);
}
//-----------------------------------------------------------------------------
// read an incoming message from datalink to network layer
void read_network(int link, size_t length, char * datagram) {
    DATAGRAM dtg;
    memcpy(&dtg, datagram, length);

    // dispatch the datagram
    if (is_kind(dtg.kind,__DISCOVER__))
        do_discovery(link, dtg);
    if (is_kind(dtg.kind,__ROUTING__))
        do_routing(link, dtg);
    if (is_kind(dtg.kind,__TRANSPORT__)) {
        PACKET pkt;
        size_t len_to_cpy = dtg.length;
        memcpy((char*)&pkt, dtg.payload, len_to_cpy);
        if ((CnetAddr)(dtg.dest) != nodeinfo.address) {
            route(dtg);
            packets_forwarded_total++;
        } else
            read_transport(dtg.kind, dtg.length, dtg.src, (char*)dtg.payload);
    }
}
//-----------------------------------------------------------------------------
// allocate a datagram
DATAGRAM alloc_datagram(uint8_t prot, int src, int dest, char *p, uint16_t len) {
    DATAGRAM np;
    np.kind = prot;
    np.src = src;
    np.dest = dest;
    np.length = len;

    size_t len_to_copy = len;
    memcpy((char*)np.payload, (char*)p, len_to_copy);
    return np;
}
//-----------------------------------------------------------------------------
// send a packet to address
void send_packet(CnetAddr addr, DATAGRAM datagram) {
    // get link to send
    int link = get_next_link_for_dest(addr);
    send_packet_to_link(link, datagram);
}
//-----------------------------------------------------------------------------
// send a packet to the link
void send_packet_to_link(int link, DATAGRAM datagram) {
    // send packet down to link layer
    size_t size = DATAGRAM_SIZE(datagram);
    write_datalink(link, (char *) &datagram, size);
}
//-----------------------------------------------------------------------------
// broadcast datagram to all links(excluded one)
void broadcast_packet(DATAGRAM dtg, int exclude_link) {
    for (int i = 1; i <= nodeinfo.nlinks; i++) {
        if (i != exclude_link) {
            send_packet_to_link(i, dtg);
        }
    }
}
//-----------------------------------------------------------------------------
void shutdown_network() {
    histogram();
    shutdown_routing();
    shutdown_datalink();
}
//-----------------------------------------------------------------------------


