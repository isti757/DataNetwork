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

#define SHORT_DEBUG

static void DEBUG1(char *description)
{
	#if defined(SHORT_DEBUG)
		printf("%c", *description);
	#endif
}

static void DEBUG2(char *description, int seq)
{
	#if defined(SHORT_DEBUG)
		printf("%c sequence no. : %d", *description, seq);
	#endif
}

//static void DEBUG3(char *description, int seq, int seg)
//{
//#if defined(SHORT_DEBUG)
//	printf("%c - sequence no. : %d segmentation no.: %d", *description, seq, seg);
//#endif
//}

#endif // APPLICATION_LAYER_H_
