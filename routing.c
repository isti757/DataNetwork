/*
 * routing.c
 *
 *  Created on: 02.08.2011
 *      Author: kirill
 */
#include "routing.h"
//-----------------PRIVATE FUNCTIONS-------------------------------------------
/*
  initialize the routing protocol
*/
void doroute();
void init_routing() {
	route_table	= (ROUTE_TABLE *)malloc(sizeof(ROUTE_TABLE));
	route_table_size = 0;
	history_table = (HISTORY_TABLE*)malloc(sizeof(HISTORY_TABLE));
	history_table_size = 0;
	reverse_route = (REVERSE_ROUTE_TABLE*)malloc(sizeof(REVERSE_ROUTE_TABLE));
	reverse_table_size = 0;
	route_req_id = 0;
	CHECK(CNET_set_handler(EV_DEBUG1,show_table, 0));
    CHECK(CNET_set_debug_string( EV_DEBUG1, "NL info"));

    CHECK(CNET_set_handler(EV_DEBUG2,doroute, 0));
    CHECK(CNET_set_debug_string( EV_DEBUG2, "ROUTE"));
}
//-----------------------------------------------------------------------------
//Find the address in the routing table
static int find_address(CnetAddr address)
{
    int	t;

    for(t=0 ; t<route_table_size ; ++t)
	if(route_table[t].address == address)
	    return(t);

    route_table	= (ROUTE_TABLE *)realloc((char *)route_table,
					(route_table_size+1)*sizeof(ROUTE_TABLE));

    route_table[route_table_size].address		= address;
    route_table[route_table_size].ackexpected	= 0;
    route_table[route_table_size].nextpackettosend	= 0;
    route_table[route_table_size].packetexpected	= 0;
    route_table[route_table_size].minhops		= MAXHOPS;
    route_table[route_table_size].minhop_link	= 0;
    route_table[route_table_size].totaltime		= 0;
    return(route_table_size++);
}
int address_exists_in_route_table(CnetAddr addr)
{
	int t;
	for (t = 0; t < route_table_size; ++t) {
		if (route_table[t].address == addr) {
			return 1;
		}
	}
	return 0;
}


static int ackexpected(CnetAddr address) {
    return(route_table[ find_address(address) ].ackexpected);
}

static void inc_ackexpected(CnetAddr address) {
    route_table[ find_address(address) ].ackexpected++;
}

static int nextpackettosend(CnetAddr address) {
    return(route_table[ find_address(address) ].nextpackettosend++);
}

static int packetexpected(CnetAddr address) {
    return(route_table[ find_address(address) ].packetexpected);
}

static void inc_packetexpected(CnetAddr address) {
    route_table[ find_address(address) ].packetexpected++;
}

static int maxhops(CnetAddr address) {
    return(route_table[ find_address(address) ].minhops);
}
static int whichlink(CnetAddr address) {
    int link	= route_table[ find_address(address) ].minhop_link;
    return link;
    //return(link == 0 ? ALL_LINKS : (1 << link));
}
//-----------------------------------------------------------------------------
//learn routing table
void learn_route_table(CnetAddr address, int hops, int link, CnetTime usec)
{
    int	t;
    t = find_address(address);
    //if(route_table[t].minhops >= hops) {
	route_table[t].minhops	= hops;
	route_table[t].minhop_link	= link;
    //}
    route_table[t].totaltime = route_table[t].totaltime+usec;
}
//-----------------------------------------------------------------------------
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
    write_datalink(link, (char *)datagram, length);
    return(0);
}
int up_to_network(DATAGRAM datagram, int length, int arrived_on)
{
	DATAGRAM *p;
	CnetAddr addr;
	p = &datagram;

	if (p->dest == nodeinfo.address) { /*  THIS PACKET IS FOR ME */
		if (p->kind == DATA && p->seqno == packetexpected(p->src)) {
			//length	= p->packet.len;
			//Pass a datagram to the transport level
			read_transport(datagram);
			inc_packetexpected(p->src);

			learn_route_table(p->src, p->hopstaken, arrived_on, 0);

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
			learn_route_table(p->src, p->hopstaken, arrived_on, took);
		}
	} else { /* THIS PACKET IS FOR SOMEONE ELSE */
		if (--p->hopsleft > 0) { /* send it back out again */
			p->hopstaken++;
			learn_route_table(p->src, p->hopstaken, arrived_on, 0);
			selective_flood((char*) &datagram, length, whichlink(p->dest) & ~(1<< arrived_on));
		}
	}
	return (0);
}


/*
   Take a packet from the transport layer and send it to
   the destination via the best route.
*/
void route (CnetAddr addr, 	DATAGRAM dtg)
{
	dtg.src = nodeinfo.address;
	dtg.kind = DATA;
	dtg.hopsleft = maxhops(dtg.dest);
	dtg.hopstaken = 1;
	dtg.timesent = nodeinfo.time_in_usec;
	dtg.seqno = nextpackettosend(dtg.dest);
	selective_flood((char *) &dtg, DATAGRAM_SIZE(dtg), whichlink(dtg.dest));


}
void show_table(CnetEvent ev, CnetTimerID timer, CnetData data)
{
    int	t;

    //CNET_clear();
    printf("table_size=%d\n",route_table_size);
    printf("\n%14s %14s %14s %14s %14s\n",
    "destination","packets-ack'ed","min-hops","minhop-link", "round-trip-time");
    for (t = 0; t < route_table_size; ++t) {
		//if ((n = table[t].ackexpected) > 0) {
			//CnetTime avtime;
			//avtime = (CnetTime) n;
			//avtime = table[t].totaltime / avtime;
			printf("%14d %14d %14ld %14ld %14llu\n", (int) route_table[t].address,
					route_table[t].ackexpected, route_table[t].minhops,
					route_table[t].minhop_link, route_table[t].totaltime);
		//}
	}

    printf("history_table_size=%d\n",history_table_size);
    printf("%14s %14s\n","source","id");
    for (t = 0; t < history_table_size; ++t) {
    	printf("%14d %14d\n", (int) history_table[t].source,history_table[t].req_id);
    }

    printf("reverse_table_size=%d\n",reverse_table_size);
    printf("%14s %14s\n","source","link");
    for (t = 0; t < reverse_table_size; ++t) {
       	printf("%14d %14d\n", (int) reverse_route[t].source,reverse_route[t].link);
    }
}

int find_local_history(CnetAddr dest, int req_id) {
	int t;
	for (t = 0; t < history_table_size; ++t){
		if ((history_table[t].source == dest) && (history_table[t].req_id==req_id)){
			return (t);
		}
	}
	return -1;
}
int insert_local_history(CnetAddr addr,int req_id) {
	 history_table	= (HISTORY_TABLE *)realloc((char *)history_table,
						(history_table_size+1)*sizeof(HISTORY_TABLE));
	 history_table[history_table_size].source = addr;
	 history_table[history_table_size].req_id = req_id;
	 return(history_table_size++);
}
int exist_reverse_route(CnetAddr address, int link) {
	int t;
	for (t = 0; t < reverse_table_size; ++t)
		if (reverse_route[t].source == address && reverse_route[t].link==link)
			return (t);
	return -1;
}
int insert_reverse_route(CnetAddr address, int link) {
	reverse_route = (REVERSE_ROUTE_TABLE *) realloc((char *) reverse_route,
			(reverse_table_size + 1) * sizeof(REVERSE_ROUTE_TABLE));
	reverse_route[reverse_table_size].source = address;
	reverse_route[reverse_table_size].link = link;
	return (reverse_table_size++);
}
int find_reverse_route(CnetAddr address) {
	int t;
	for (t = 0; t < reverse_table_size; ++t)
		if (reverse_route[t].source == address)
			return reverse_route[t].link;
	return -1;
}

void do_routing(int link,DATAGRAM datagram)
{
	ROUTE_PACKET r_packet;
	memcpy(&r_packet,&datagram.payload,datagram.length);
	switch (r_packet.type) {
		case RREQ:
			printf("Received rreq on %d link=%d\n",nodeinfo.address,link);
			printf("hop count = %d\n",r_packet.hop_count);
			//check local history if we already processed this RREQ
			if (find_local_history(r_packet.source,r_packet.req_id) != -1) {
				break;
			}
			printf("no entry in local history\n");
			//insert into local history table
			insert_local_history(r_packet.source,r_packet.req_id);
			//look into our route table
			if (address_exists_in_route_table(r_packet.dest) || nodeinfo.address==r_packet.dest) {
				//create RREP
				printf("Sending RREP from %d to link %d\n",nodeinfo.address,link);
				ROUTE_PACKET reply_packet;
				reply_packet.type = RREP;
				reply_packet.source = r_packet.source;
				reply_packet.dest = r_packet.dest;
				reply_packet.hop_count = 0;
				DATAGRAM* reply_datagram = alloc_datagram(ROUTING,r_packet.source,r_packet.dest,
						(char*)&reply_packet,sizeof(reply_packet));
				send_packet_to_link(link,*reply_datagram);
				break;
			}
			//the receiver doesn't know
			//increment hop count
			r_packet.hop_count = r_packet.hop_count+1;
			//rebroadcast the message
			DATAGRAM* r_datagram = alloc_datagram(ROUTING,r_packet.source,r_packet.dest,
					(char*)&r_packet,sizeof(r_packet));
			broadcast_packet(*r_datagram,link);
			//store new entry in reverse table
			//no entry in the table now
			if (exist_reverse_route(r_packet.source, link) == -1) {
				insert_reverse_route(r_packet.source, link);
			}
			break;
		case RREP:
			printf("Received rrep on %d link=%d\n",nodeinfo.address,link);
			printf("hop count = %d\n",r_packet.hop_count);
			r_packet.hop_count = r_packet.hop_count+1;
			if (address_exists_in_route_table(r_packet.dest) == 0) {
				learn_route_table(r_packet.dest,r_packet.hop_count,link,0); //TODO usec!
			}
			//if the packet didn't reach its destination
			if (nodeinfo.address != r_packet.source) {
				printf("Send rrep to reverse route\n");
				int reverse_route_link_id = find_reverse_route(r_packet.source);
				if (reverse_route_link_id==-1) {
					printf("BIG bug link==-1");
					break;
				}
				DATAGRAM* reply_datagram = alloc_datagram(ROUTING,r_packet.source,r_packet.dest,
										(char*)&r_packet,sizeof(r_packet));
				send_packet_to_link(reverse_route_link_id,*reply_datagram);
			}
			break;
	}
}

//-----------------------------------------------------------------------------
// detect a link for outcoming message
int get_next_link_for_dest(CnetAddr destaddr)
{
	//address not exists yet
	if (address_exists_in_route_table(destaddr)==0) {
		printf("the address is not in route table\n");
		ROUTE_PACKET req_packet;
		req_packet.type = RREQ;
		req_packet.source = nodeinfo.address;
		req_packet.dest = destaddr;
		req_packet.hop_count = 0;
		req_packet.req_id = route_req_id;
		route_req_id++;
		DATAGRAM* r_packet = alloc_datagram(ROUTING,nodeinfo.address,destaddr,(char*)&req_packet,sizeof(req_packet));
		printf("sending rreq for %d\n",destaddr);
		broadcast_packet(*r_packet,-1);
		return 0;
	} else {
		printf("the address is in route table link=%d\n",whichlink(destaddr));
		return whichlink(destaddr);
	}
}
//-----------------------------------------------------------------------------
// detect fragmentation size for the specified link
int get_mtu_for_link(int link)
{
	return 96;
}
void doroute() {
	if (nodeinfo.address==0) {
		get_next_link_for_dest(5);
	}
}


