

#include "socal/socal.h"

#include "scu.h"

// SCU location in ARM memory-map: Defined in the CycloneV HPS manual,
// cv_5v4 section 9.
#define SCU_BASE    0xfffec000U

// SCU registers: Defined in the ARM Cortex-A( MPCore TRM,
// ARM DDI 0407G ID0
#define SCU_CTL     SCU_BASE
#define SCU_CFG     (SCU_BASE+0x04U)
#define SCU_POW     (SCU_BASE+0x08U)
#define SCU_INV     (SCU_BASE+0x0cU)
#define SCU_FSA     (SCU_BASE+0x40U)
#define SCU_FEA     (SCU_BASE+0x44U)
#define SCU_SAC     (SCU_BASE+0x50U)
#define SCU_SNSAC   (SCU_BASE+0x54U)

uint32_t scu_getCtl() {
  return alt_read_word( SCU_CTL );
}

uint32_t scu_getCfg() {
  return alt_read_word( SCU_CFG );
}

void scu_setCtl( uint32_t val ) {
  alt_write_word( SCU_CTL, val );
}

void scu_setCfg( uint32_t val ) {
  alt_write_word( SCU_CFG, val );
}

// Do this between invalidating L1 and joining SMP.
// cpu:0..3; ways:0..0xf.
void scu_secure_invalidate(uint32_t cpu, uint32_t ways) {
  // Write the 4bit 'ways' field shifted left by 4x cpu num.
  ways &= 0xfU;
  ways <<= ((cpu & 0x3U)<<2);
  alt_write_word( SCU_INV, ways );
}

