
#ifndef _H_SCU
#define _H_SCU

#include <inttypes.h>


uint32_t scu_getCtl();

uint32_t scu_getCfg();

void scu_setCtl( uint32_t val );

void scu_setCfg( uint32_t val );

void scu_secure_invalidate(uint32_t cpu, uint32_t ways);

#endif


