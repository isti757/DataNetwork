/*
 * network_layer.c
 *
 *  Created on: Jul 30, 2011
 *      Author: isti
 */
#include <cnet.h>
#include <stdlib.h>
#include "network_layer.h"

#define	MAXHOPS		4
#define	ALL_LINKS	(-1)




/**
 * A network routing table for each node
 */
typedef struct {
    CnetAddr	address;
    int		ackexpected;
    int		nextpackettosend;
    int		packetexpected;
    long	minhops;
    long	minhop_link;
    CnetTime	totaltime;
} NL_TABLE;

static	NL_TABLE	*table;
static	int		table_size;

//-----------------PRIVATE FUNCTIONS-------------------------------------------
//initialize the network table
void init_network() {
	table	= (NL_TABLE *)malloc(sizeof(NL_TABLE));
	table_size	= 0;
}
//-----------------------------------------------------------------------------
//Find the address in the routing table
static int find_address(CnetAddr address)
{
    int	t;

    for(t=0 ; t<table_size ; ++t)
	if(table[t].address == address)
	    return(t);

    table	= (NL_TABLE *)realloc((char *)table,
					(table_size+1)*sizeof(NL_TABLE));

    table[table_size].address		= address;
    table[table_size].ackexpected	= 0;
    table[table_size].nextpackettosend	= 0;
    table[table_size].packetexpected	= 0;
    table[table_size].minhops		= MAXHOPS;
    table[table_size].minhop_link	= 0;
    table[table_size].totaltime		= 0;
    return(table_size++);
}
static int ackexpected(CnetAddr address) {
    return(table[ find_address(address) ].ackexpected);
}

static void inc_ackexpected(CnetAddr address) {
    table[ find_address(address) ].ackexpected++;
}

static int nextpackettosend(CnetAddr address) {
    return(table[ find_address(address) ].nextpackettosend++);
}

static int packetexpected(CnetAddr address) {
    return(table[ find_address(address) ].packetexpected);
}

static void inc_packetexpected(CnetAddr address) {
    table[ find_address(address) ].packetexpected++;
}

static int maxhops(CnetAddr address) {
    return(table[ find_address(address) ].minhops);
}
static int whichlink(CnetAddr address) {
    int link	= table[ find_address(address) ].minhop_link;
    return(link == 0 ? ALL_LINKS : (1 << link));
}

static void NL_stats(CnetAddr address, int hops, int link, CnetTime usec)
{
    int	t;

    t = find_address(address);
    if(table[t].minhops >= hops) {
	table[t].minhops	= hops;
	table[t].minhop_link	= link;
    }
    table[t].totaltime = table[t].totaltime+usec;
}

static int down_to_datalink(int link, char *datagram, int length);

static void selective_flood(char *packet, int length, int links_wanted)
{
    int	   link;

    for(link=1 ; link<=nodeinfo.nlinks ; ++link)
	if( links_wanted & (1<<link) )
	    CHECK(down_to_datalink(link, packet, length));
}

static int down_to_datalink(int link, char *datagram, int length)
{
    CHECK(CNET_write_physical(link, (char *)datagram, (size_t*)&length));
    return(0);
}
static int up_to_network(DATAGRAM datagram, int length, int arrived_on) {
	DATAGRAM *p;
	CnetAddr addr;
	p = &datagram;

	if (p->dest == nodeinfo.address) { /*  THIS PACKET IS FOR ME */
		if (p->kind == DATA && p->seqno == packetexpected(p->src)) {
			//length	= p->packet.len;
			//Pass a datagram to the transport level
			write_transport(datagram);
			inc_packetexpected(p->src);

			NL_stats(p->src, p->hopstaken, arrived_on, 0);

			addr = p->src; /* transform NL_DATA into NL_ACK */
			p->src = p->dest;
			p->dest = addr;

			p->kind = ACK;
			p->hopsleft = maxhops(p->dest);
			p->hopstaken = 1;
			p->length = 0;
			selective_flood((char*) &datagram, DATAGRAM_HEADER_SIZE, whichlink(p->dest));
		} else if (p->kind == ACK && p->seqno == ackexpected(p->src)) {
			CnetTime took;
			CNET_enable_application(p->src);
			inc_ackexpected(p->src);
			took = nodeinfo.time_in_usec - p->timesent;
			NL_stats(p->src, p->hopstaken, arrived_on, took);
		}
	} else { /* THIS PACKET IS FOR SOMEONE ELSE */
		if (--p->hopsleft > 0) { /* send it back out again */
			p->hopstaken++;
			NL_stats(p->src, p->hopstaken, arrived_on, 0);
			selective_flood((char*) &datagram, length, whichlink(p->dest) & ~(1<< arrived_on));
		}
	}
	return (0);
}

void show_table(CnetEvent ev, CnetTimerID timer, CnetData data)
{
    int	t, n;

    CNET_clear();
    printf("table_size=%d\n",table_size);
    printf("\n%14s %14s %14s %14s %14s\n",
    "destination","packets-ack'ed","min-hops","minhop-link", "round-trip-time");
    for(t=0 ; t<table_size ; ++t)
	if((n = table[t].ackexpected) > 0) {
	    CnetTime	avtime;

	    avtime = (CnetTime)n;
	    avtime = table[t].totaltime/avtime;
	    printf("%14d %14d %14ld %14ld %14llu\n",
		    (int)table[t].address, table[t].ackexpected,
		    table[t].minhops, table[t].minhop_link,
		    avtime);
	}
}

//-----------------END OF PRIVATE FUNCTIONS------------------------------------
//-----------------------------------------------------------------------------
// read an incoming packet into network layer
void read_network(DATAGRAM dtg) {


	dtg.src = nodeinfo.address;
	dtg.kind = DATA;
	dtg.hopsleft = maxhops(dtg.dest);
	dtg.hopstaken = 1;
	dtg.timesent = nodeinfo.time_in_usec;
	dtg.seqno = nextpackettosend(dtg.dest);


	selective_flood((char *) &dtg, DATAGRAM_SIZE(dtg), whichlink(dtg.dest));

	//read_datalink(link, dtg);
}
//-----------------------------------------------------------------------------
// write an incoming message from datalink to network layer
void write_network(int link, DATAGRAM dtg, int length)
{
	// if not this host, send further
	// update routing tables
	// send it further
	CHECK(up_to_network(dtg, length, link));
}
//-----------------------------------------------------------------------------
// detect a link for outcoming message
int get_next_link_for_dest(CnetAddr destaddr)
{
	return 1;
}
//-----------------------------------------------------------------------------
// detect fragmentation size for the specified link
int get_mtu_for_link(int link)
{
	return 96;
}
