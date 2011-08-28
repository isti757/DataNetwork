/*
 * transport_layer.c
 *
 *  Created on: Jul 30, 2011
 *      Author: isti
 */

#include "transport_layer.h"
#include "routing.h"
#include "datagram.h"
//packet queue
QUEUE packet_queue;
//flush the transport queue timer
CnetTimerID flush_transport_queue_timer;

//-----------------------------------------------------------------------------
// read incoming message from network to transport layer
void write_transport(CnetEvent ev, CnetTimerID timer, CnetData data)
{
	// read the message from application layer
	CnetAddr destaddr;
	PACKET pkt;
	pkt.segid = 0;
	pkt.seqno = 0;
	size_t packet_len = MAX_MESSAGE_SIZE;
	CHECK(CNET_read_application(&destaddr, &pkt.msg, &packet_len));
	printf("Sending packet to %d length=%d\n",destaddr,packet_len);

	//create container for outgoing message
	PKT_CONTAINER packet_container;
	packet_container.dest = destaddr;
	packet_container.len = PACKET_HEADER_SIZE+packet_len;//the lenght of pkt.msg
	size_t pkt_total_len = PACKET_HEADER_SIZE+packet_len;
	memcpy(&packet_container.packet,&pkt,pkt_total_len);

	//add to the queue
	size_t pkt_cont_len = PKT_CONTAINER_HEADER_SIZE+pkt_total_len;
	queue_add(packet_queue,&packet_container,pkt_cont_len);
}
//-----------------------------------------------------------------------------
// write incoming message from application into transport layer
void read_transport(uint8_t kind,uint16_t length,CnetAddr source,char * packet)
{
	PACKET p;
	size_t len = length;
	memcpy(&p,packet,len);
	len-=PACKET_HEADER_SIZE;
    CNET_write_application((char*)p.msg, &len);

}

//flushing the queue function
void flush_transport_queue(CnetEvent ev, CnetTimerID timer, CnetData data)
{
	printf("Number of packets in the transport queue=%d\n",queue_nitems(packet_queue));
	if (queue_nitems(packet_queue)>50) {
		CNET_disable_application(ALLNODES);
	}
	if ((queue_nitems(packet_queue)==0) && (check_neighbors_discovered()==1)) {
		//if (nodeinfo.address==182)
		CNET_enable_application(ALLNODES);
	}
	//check if the size of the queue > 0
	if (queue_nitems(packet_queue)>0) {
		size_t containter_len;
		PKT_CONTAINER * pkt_container = queue_remove(packet_queue, &containter_len);

		//there is a route to this destination
		if (get_mtu(pkt_container->dest)!=-1) {
			printf("A route exists for sending to %d\n",pkt_container->dest);
			size_t total_packet_len = pkt_container->len;
			PACKET copy_packet;
			memcpy(&copy_packet,pkt_container->packet,total_packet_len);

			write_network(__TRANSPORT__,pkt_container->dest,total_packet_len,(char*)&copy_packet);
		} else {
			//there is no route yet
			printf("A route doesn't exist for sending to %d\n",pkt_container->dest);
			//add back to the queue
			PKT_CONTAINER copy_container;
			copy_container.dest = pkt_container->dest;
			copy_container.len = pkt_container->len;
			size_t copied_packet_len = pkt_container->len;
			memcpy(&copy_container.packet,pkt_container->packet,copied_packet_len);
			size_t copy_length = PKT_CONTAINER_HEADER_SIZE+copied_packet_len;
			queue_add(packet_queue,&copy_container,copy_length);
		}
		free(pkt_container);
	}
	flush_transport_queue_timer = CNET_start_timer(EV_FLUSH_TRANSPORT_QUEUE,100000,-1);
}

//-----------------------------------------------------------------------------
// clean all allocated memory
static void shutdown(CnetEvent ev, CnetTimerID t1, CnetData data) {


	shutdown_network();
}
//-----------------------------------------------------------------------------
void init_transport()
{
	//set cnet handler
	CHECK(CNET_set_handler(EV_APPLICATIONREADY, write_transport, 0));
	CHECK(CNET_set_handler( EV_SHUTDOWN, shutdown, 0));
	CHECK(CNET_set_handler(EV_FLUSH_TRANSPORT_QUEUE, flush_transport_queue, 0));
	//init transport queue
	packet_queue = queue_new();
	flush_transport_queue_timer = CNET_start_timer(EV_FLUSH_TRANSPORT_QUEUE,100000,-1);
}
//-----------------------------------------------------------------------------
void signal_transport(SIGNALKIND sg, SIGNALDATA data) {
	if (sg==DISCOVERY_FINISHED) {
		printf("Discovery finished\n");
		CNET_enable_application(ALLNODES);
	}


}
