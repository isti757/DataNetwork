/*
 * routing.c
 *
 *  Created on: 02.08.2011
 *      Author: kirill
 */
#include "routing.h"
#include <unistd.h>

CnetTimerID route_pending_timer;
/*
  initialize the routing protocol
*/
//void doroute();
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

    /*CHECK(CNET_set_handler(EV_DEBUG2,doroute, 0));
    CHECK(CNET_set_debug_string( EV_DEBUG2, "ROUTE"));*/

    CHECK(CNET_set_handler(EV_ROUTE_PENDING_TIMER, route, 0));

    route_pending_timer = CNET_start_timer(EV_ROUTE_PENDING_TIMER,ROUTE_POLLING_TIME,0);

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

    route_table[route_table_size].address = address;
    route_table[route_table_size].minhops = 0;
    route_table[route_table_size].minhop_link = 0;
    route_table[route_table_size].total_delay = 0;
    route_table[route_table_size].min_mtu = 0;
    return(route_table_size++);
}
//-----------------------------------------------------------------------------
//check if address exists in the routing table
int is_route_exists(CnetAddr addr)
{
	int t;
	for (t = 0; t < route_table_size; ++t) {
		if (route_table[t].address == addr) {
			return 1;
		}
	}
	return 0;
}
//-----------------------------------------------------------------------------
//find a link to send datagram
static int whichlink(CnetAddr address) {
    int link	= route_table[ find_address(address) ].minhop_link;
    return link;
}
//-----------------------------------------------------------------------------
//learn routing table TODO what about best route?
void learn_route_table(CnetAddr address, int hops, int link,int mtu,CnetTime total_delay)
{
    int	t;
    t = find_address(address);
    //if(route_table[t].minhops >= hops) {
	route_table[t].minhops	= hops;
	route_table[t].minhop_link	= link;
	route_table[t].min_mtu = mtu;
	route_table[t].total_delay = total_delay;
    //}
}
//-----------------------------------------------------------------------------
/* Take a packet from the transport layer and send it to the destination */
void route(CnetEvent ev, CnetTimerID timer, CnetData data)
{
	if (queue_nitems(datagram_queue)>50) {
		CNET_disable_application(ALLNODES);
	}
	if (queue_nitems(datagram_queue)==0) {
		//if (nodeinfo.address==182)
		CNET_enable_application(ALLNODES);
	}
	printf("Checking the queue \n");
	if (queue_nitems(datagram_queue) != 0) {
		size_t len;
		DATAGRAM *dtg;
		dtg = queue_peek(datagram_queue, &len);
		int link = get_next_link_for_dest(dtg->dest);
		if (link == -1) {
			printf("in route: no address for %d sending request\n",dtg->dest);
			send_route_request(dtg->dest);
		} else {
			printf("in route: send to link %d\n to address=%d\n",link,dtg->dest);
			DATAGRAM copy_dtg;
			size_t datagram_length = DATAGRAM_SIZE((*dtg));
			memcpy((char*)&copy_dtg,(char*)dtg,datagram_length);
			send_packet_to_link(link,copy_dtg);
			queue_remove(datagram_queue,&len);
			free(dtg);
		}
	}
	route_pending_timer = CNET_start_timer(EV_ROUTE_PENDING_TIMER,ROUTE_POLLING_TIME,0);
}
//-----------------------------------------------------------------------------

void show_table(CnetEvent ev, CnetTimerID timer, CnetData data)
{
    int	t;

    //CNET_clear();
    printf("table_size=%d\n",route_table_size);
    printf("\n%14s %14s %14s %14s %14s\n",
    "destination","min-hops","minhop-link","mtu", "time");
    for (t = 0; t < route_table_size; ++t) {
		//if ((n = table[t].ackexpected) > 0) {
			//CnetTime avtime;
			//avtime = (CnetTime) n;
			//avtime = table[t].totaltime / avtime;
			printf("%14d %14d %14ld %14ld %14llu\n", (int) route_table[t].address,
					route_table[t].minhops,route_table[t].minhop_link,
					route_table[t].min_mtu, route_table[t].total_delay);
		//}
	}

    /*printf("history_table_size=%d\n",history_table_size);
    printf("%14s %14s\n","source","id");
    for (t = 0; t < history_table_size; ++t) {
    	printf("%14d %14d\n", (int) history_table[t].source,history_table[t].req_id);
    }*/

    printf("reverse_table_size=%d\n",reverse_table_size);
    printf("%14s %14s\n","source","link");
    for (t = 0; t < reverse_table_size; ++t) {
       	printf("%14d %14d\n", (int) reverse_route[t].source,reverse_route[t].link);
    }
}
//-----------------------------------------------------------------------------
//find an item in the local history
int find_local_history(CnetAddr dest, int req_id) {
	int t;
	for (t = 0; t < history_table_size; ++t){
		if ((history_table[t].source == dest) && (history_table[t].req_id==req_id)){
			return (t);
		}
	}
	return -1;
}
//-----------------------------------------------------------------------------
//insert an item in the local history
int insert_local_history(CnetAddr addr,int req_id) {
	 history_table	= (HISTORY_TABLE *)realloc((char *)history_table,
						(history_table_size+1)*sizeof(HISTORY_TABLE));
	 history_table[history_table_size].source = addr;
	 history_table[history_table_size].req_id = req_id;
	 return(history_table_size++);
}
//-----------------------------------------------------------------------------
//check if reverse route exists
int exist_reverse_route(CnetAddr address, int link) {
	int t;
	for (t = 0; t < reverse_table_size; ++t)
		if (reverse_route[t].source == address && reverse_route[t].link==link)
			return (t);
	return -1;
}
//-----------------------------------------------------------------------------
//insert an item into the reverse route table
int insert_reverse_route(CnetAddr address, int link) {
	reverse_route = (REVERSE_ROUTE_TABLE *) realloc((char *) reverse_route,
			(reverse_table_size + 1) * sizeof(REVERSE_ROUTE_TABLE));
	reverse_route[reverse_table_size].source = address;
	reverse_route[reverse_table_size].link = link;
	return (reverse_table_size++);
}
//-----------------------------------------------------------------------------
//find a reverse route link
int find_reverse_route(CnetAddr address) {
	int t;
	for (t = 0; t < reverse_table_size; ++t)
		if (reverse_route[t].source == address)
			return reverse_route[t].link;
	return -1;
}
//-----------------------------------------------------------------------------
//process a routing datagram
void do_routing(int link,DATAGRAM datagram)
{
	ROUTE_PACKET r_packet;
	size_t datagram_length = datagram.length;
	memcpy(&r_packet,&datagram.payload,datagram_length);
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
			//if (is_route_exists(r_packet.dest) || nodeinfo.address==r_packet.dest) {
			if (nodeinfo.address==r_packet.dest) {
				//create RREP
				printf("Sending RREP from %d to link %d\n",nodeinfo.address,link);
				ROUTE_PACKET reply_packet;
				reply_packet.type = RREP;
				reply_packet.source = r_packet.source;
				reply_packet.dest = r_packet.dest;
				reply_packet.hop_count = 0;
				reply_packet.total_delay = linkinfo[link].propagationdelay;
				reply_packet.min_mtu = linkinfo[link].mtu;
				uint16_t r_size = sizeof(reply_packet);
				DATAGRAM* reply_datagram = alloc_datagram(ROUTING,r_packet.source,r_packet.dest,
						(char*)&reply_packet,r_size);
				send_packet_to_link(link,*reply_datagram);
				break;
			}
			//the receiver doesn't know
			//increment hop count
			r_packet.hop_count = r_packet.hop_count+1;
			//rebroadcast the message
			uint16_t request_size = sizeof(r_packet);
			DATAGRAM* r_datagram = alloc_datagram(ROUTING,r_packet.source,r_packet.dest,
					(char*)&r_packet,request_size);
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
			if (is_route_exists(r_packet.dest) == 0) {
				learn_route_table(r_packet.dest,r_packet.hop_count,link,r_packet.min_mtu,r_packet.total_delay); //TODO usec!
			}
			//if the packet didn't reach its destination
			if (nodeinfo.address != r_packet.source) {
				printf("Send rrep to reverse route\n");
				int reverse_route_link_id = find_reverse_route(r_packet.source);
				if (reverse_route_link_id==-1) {
					printf("Link for reverse routing RREP==-1");
					break;
				}
				//set up propagation delay and mtu
				r_packet.total_delay = r_packet.total_delay+linkinfo[reverse_route_link_id].propagationdelay;
				if (r_packet.min_mtu>linkinfo[reverse_route_link_id].mtu) {
					r_packet.min_mtu = linkinfo[reverse_route_link_id].mtu;
				}
				uint16_t reply_size = sizeof(r_packet);
				DATAGRAM* reply_datagram = alloc_datagram(ROUTING,r_packet.source,r_packet.dest,
										(char*)&r_packet,reply_size);
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
	if (is_route_exists(destaddr)==0) {
		printf("the address is not in route table\n");
		return -1;
	} else {
		printf("the address is in route table link=%d\n",whichlink(destaddr));
		return whichlink(destaddr);
	}
}
//-----------------------------------------------------------------------------
//send a route request
void send_route_request(CnetAddr destaddr) {
	ROUTE_PACKET req_packet;
	req_packet.type = RREQ;
	req_packet.source = nodeinfo.address;
	req_packet.dest = destaddr;
	req_packet.hop_count = 0;
	req_packet.req_id = route_req_id;
	route_req_id++;
	uint16_t request_size = sizeof(req_packet);
	DATAGRAM* r_packet = alloc_datagram(ROUTING, nodeinfo.address, destaddr,
			(char*) &req_packet, request_size);
	printf("sending rreq for %d\n", destaddr);
	broadcast_packet(*r_packet, -1);
}
//-----------------------------------------------------------------------------
// detect fragmentation size for the specified destination
int get_mtu_for_route(CnetAddr address)
{
	if (is_route_exists(address)==1) {
		int t = find_address(address);
		return route_table[t].min_mtu;
	}
	return -1;
}
/*void doroute() {
	if (nodeinfo.address==0) {
		get_next_link_for_dest(5);
	}
}*/
//-----------------------------------------------------------------------------
//check if all neighbors have been discovered
int check_neighbors_discovered() {
	if (route_table_size==nodeinfo.nlinks) {
		return 1;
	}
	return 0;
}

