
#ifndef _H_CTASSERT
#define _H_CTASSERT

#include "baremetal.h"

// Compile-Time Assertion.  Usage example: CTASSERT(sizeof(x)==Y,badxSize);
#define CTASSERT(PRED,NAME)     char ctassert_##NAME[PRED?0:-1]

// The value is a power of 2.
#define CTASSERT_ISPOW2(X,NAME)   CTASSERT((X>0)&&(0==(X&(X-1))),NAME)

// Is the value a whole number of cache-lines?
// Note the value is on bytes so if sizing for arrays factor in the element size.
#define CTASSERT_SIZE_ISCACHELINES(X,NAME) \
     CTASSERT( (X) == ((X)&(~(BM_CACHE_LINE-1))), NAME )

// Is the declaration a whole number of cache-lines?
#define CTASSERT_DECL_ISCACHELINES(X,NAME) \
     CTASSERT_SIZE_ISCACHELINES(sizeof(X),NAME)
//     CTASSERT( sizeof(X) == (sizeof(X)&(~(BM_CACHE_LINE-1))), NAME )

#endif

