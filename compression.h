#ifndef COMPRESSION_H_
#define COMPRESSION_H_


#include <stdio.h>
#include <stdlib.h>

#include "minilzo.h"
#include "minilzo.c"

#if defined(__LZO_STRICT_16BIT)
#define IN_LEN      (8*1024u)
#elif defined(LZO_ARCH_I086) && !defined(LZO_HAVE_MM_HUGE_ARRAY)
#define IN_LEN      (60*1024u)
#else
#define IN_LEN      (128*1024ul)
#endif
#define OUT_LEN     (IN_LEN + IN_LEN / 16 + 64 + 3)

static unsigned char __LZO_MMODEL in  [ IN_LEN ];
static unsigned char __LZO_MMODEL out [ OUT_LEN ];

#define HEAP_ALLOC(var,size) \
    lzo_align_t __LZO_MMODEL var [ ((size) + (sizeof(lzo_align_t) - 1)) / sizeof(lzo_align_t) ]

static HEAP_ALLOC(wrkmem, LZO1X_1_MEM_COMPRESS);

#endif // COMPRESSION_H_
