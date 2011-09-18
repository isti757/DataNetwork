/*
 * debug.h
 *
 *  Created on: Aug 30, 2011
 *      Author: isti
 */

#ifndef DEBUG_H_
#define DEBUG_H_

#define TRANSPORT_LAYER_DEBUG
#define NETWORK_LAYER_DEBUG


#ifdef TRANSPORT_LAYER_DEBUG
#   define T_DEBUG(msg) printf(msg)
#   define T_DEBUG1(msg, arg1) printf(msg, arg1)
#   define T_DEBUG2(msg, arg1, arg2) printf(msg, arg1, arg2)
#   define T_DEBUG3(msg, arg1, arg2, arg3) printf(msg, arg1, arg2, arg3)
#   define T_DEBUG4(msg, arg1, arg2, arg3, arg4) printf(msg, arg1, arg2, arg3, arg4)
#else
#   define T_DEBUG(msg)
#   define T_DEBUG1(msg, arg1)
#   define T_DEBUG2(msg, arg1, arg2)
#   define T_DEBUG3(msg, arg1, arg2, arg3)
#   define T_DEBUG4(msg, arg1, arg2, arg3, arg4)
#endif

#ifdef NETWORK_LAYER_DEBUG
#   define N_DEBUG(msg) printf(msg)
#   define N_DEBUG1(msg, arg1) printf(msg, arg1)
#   define N_DEBUG2(msg, arg1, arg2) printf(msg, arg1, arg2)
#   define N_DEBUG3(msg, arg1, arg2, arg3) printf(msg, arg1, arg2, arg3)
#   define N_DEBUG4(msg, arg1, arg2, arg3, arg4) printf(msg, arg1, arg2, arg3, arg4)
#   define N_DEBUG5(msg, arg1, arg2, arg3, arg4, arg5) printf(msg, arg1, arg2, arg3, arg4, arg5)
#else
#   define N_DEBUG(msg)
#   define N_DEBUG1(msg, arg1)
#   define N_DEBUG2(msg, arg1, arg2)
#   define N_DEBUG3(msg, arg1, arg2, arg3)
#   define N_DEBUG4(msg, arg1, arg2, arg3, arg4)
#   define N_DEBUG5(msg, arg1, arg2, arg3, arg4, arg5)
#endif

#ifdef DATALINK_LAYER_DEBUG
#   define D_DEBUG(msg) printf(msg)
#   define D_DEBUG1(msg, arg1) printf(msg, arg1)
#   define D_DEBUG2(msg, arg1, arg2) printf(msg, arg1, arg2)
#   define D_DEBUG3(msg, arg1, arg2, arg3) printf(msg, arg1, arg2, arg3)
#   define D_DEBUG4(msg, arg1, arg2, arg3, arg4) printf(msg, arg1, arg2, arg3, arg4)
#else
#   define D_DEBUG(msg)
#   define D_DEBUG1(msg, arg1)
#   define D_DEBUG2(msg, arg1, arg2)
#   define D_DEBUG3(msg, arg1, arg2, arg3)
#   define D_DEBUG4(msg, arg1, arg2, arg3, arg4)
#endif

#endif /* DEBUG_H_ */
