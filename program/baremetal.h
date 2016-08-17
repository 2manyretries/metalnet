
#ifndef _H_BAREMETAL
#define _H_BAREMETAL

// Require: Power of 2.
#define BM_CACHE_LINE  32U

#define BM_ALIGN(X)  __attribute__ ((aligned ( X )))

// Cache align.
#define BM_CACHE_ALIGN  BM_ALIGN(BM_CACHE_LINE)

// Round X in bytes up to fill its last cache line.
// Only really useful if data also BM_CACHE_ALIGNed.
#define BM_CACHE_ALIGNED_SIZE8(X)  ((X+BM_CACHE_LINE-1U)&(~(BM_CACHE_LINE-1U)))

// ----
/// Whether Cache Control is explicit (programmed).
/// If the system is set up without caches or with HW-cache-coherency then
/// this should be undefined.
/// If the system is set up so that Cores/peripherals do not have coherency
/// then this should be defined.
/// Furthermore the routines defined should be used as necessary to enforce
/// explicit coherency.
#define BM_EXPLICIT_CACHE_CTRL
//
#ifdef BM_EXPLICIT_CACHE_CTRL
#include "alt_cache.h"
#define BM_CACHE_INVALIDATE(AT,FOR)    alt_cache_system_invalidate( AT, FOR )
#define BM_CACHE_PUSH(AT,FOR)          alt_cache_system_clean( AT, FOR )
#define BM_CACHE_PUSHOUT(AT,FOR)       alt_cache_system_purge( AT, FOR )
#else
#define BM_CACHE_INVALIDATE(AT,FOR)    ((void)0)
#define BM_CACHE_PUSH(AT,FOR)          ((void)0)
#define BM_CACHE_PUSHOUT(AT,FOR)       ((void)0)
#endif
// ----

#endif
