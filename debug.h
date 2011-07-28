/*
 * debug.h
 *
 *  Created on: Jul 15, 2011
 *      Author: isti
 */

#ifndef DEBUG_LAYER_H_
#define DEBUG_LAYER_H_

#include <stdio.h>
#include <string.h>

#include <cnet.h>

#define SHORT_DEBUG

static void DEBUG1(char *description)
{
	#if defined(SHORT_DEBUG)
		printf("%c\n\n", *description);
	#endif
}

static void DEBUG2(char *description, int seq)
{
	#if defined(SHORT_DEBUG)
		printf("%c sequence no. : %d\n\n", *description, seq);
	#endif
}

static void DEBUG_NODE_STATISTICS()
{
#if defined(SHORT_DEBUG)
	printf("Node statistics: %s\n", nodeinfo.nodename);
	printf("message rate: %d\n\n", nodeinfo.messagerate);
#endif
}

#endif // APPLICATION_LAYER_H_
