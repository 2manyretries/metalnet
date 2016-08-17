
#include <stdio.h>

#include "socal/hps.h"
#include "socal/socal.h"
#include "alt_clock_manager.h"


// Customisation - our EMAC instance.
#include "emac_cfg.h"

#include "emac_sys.h"

// The clock is to match with the EMAC.
#if   0==EMAC_CFG_NUM
#define  EMAC_CFG_CLK     ALT_CLK_EMAC0
#elif 1==EMAC_CFG_NUM
#define  EMAC_CFG_CLK     ALT_CLK_EMAC1
#endif


//
//#define CLKMGR_BASE         0xffd04000U
//#define CLKMGR_PPLL_VCO     (CLKMGR_BASE+0x80U)
//#define CLKMGR_PPLL_EMAC(N) (CLKMGR_BASE+0x88U+(4U*N))
//#define CLKMGR_PPLL_EN      (CLKMGR_BASE+0xa0U)


//
#define RSTMGR_BASE         0xffd05000U
#define RSTMGR_PERMODRST    (RSTMGR_BASE+0x14U)

//
#define SYSMGR_BASE         0xffd08000U
#define SYSMGR_FPGA_MOD     (SYSMGR_BASE+0x28U)
#define SYSMGR_EMAC_CTRL    (SYSMGR_BASE+0x60U)
#define SYSMGR_EMAC_L3      (SYSMGR_BASE+0x64U)



/// Perform system level initialisation w.r.t the MAC.
/// Ostensibly the preloader shall have already done this.
/// NOTE: However the default ctrl setting is invalid! We need to fix it.
bool emac_sys_init() {
  bool ok = true;

/*** The following works but is done more succintly by alt_clk_freq_get() below TODO:TBC.
  // We want the EMAC peripheral to be clocked at 250MHz.
  // Experiment suggests Preloader does this for us so just check here.
  // We assume the base clk freq and check the derived EMAC clock formula.
  const uint32_t base_clk_mhz = 25U;
  const uint32_t vco = alt_read_word(CLKMGR_PPLL_VCO);
  const uint32_t vco_mul = 1U+((vco&0xfff8)>>3);
  const uint32_t vco_div = 1U+((vco&0x3f0000)>>16);
  const uint32_t emacclk = alt_read_word(CLKMGR_PPLL_EMAC(EMAC_CFG_NUM));
  const uint32_t emacclk_div = 1U+emacclk;
  ok = (vco_mul>0) && (vco_div>0) && (emacclk_div>0);
  const uint32_t emac_clk_mhz = ok?(base_clk_mhz*vco_mul/(vco_div*emacclk_div)):0;
  if(250U != emac_clk_mhz) {
    fprintf(stderr,"WARN:EMAC Clk looks wrong:\r\n");
    fprintf(stderr,"\tvco:0x%08x:25MHz*%u/%u\r\n",(unsigned)vco, (unsigned)vco_mul, (unsigned)vco_div );
    fprintf(stderr,"\temacclk:0x%08x:vco/%u\r\n",(unsigned)emacclk, (unsigned)emacclk_div );
  }
  fprintf(stderr,"clk.en:0x%08x\r\n", (unsigned)alt_read_word(CLKMGR_PPLL_EN) );
***/

  //----
  // We need the EMAC peripheral configured for RGMII.
  // Manual says this is not the default (indeed the default is invalid!) and
  // experiment suggests bootloader does NOT fix it up - we must do so...
  const uint32_t module = alt_read_word(SYSMGR_FPGA_MOD);
  const uint32_t ctrl = alt_read_word(SYSMGR_EMAC_CTRL);
  const uint32_t l3   = alt_read_word(SYSMGR_EMAC_L3);
  fprintf(stderr,"sysmgr:ctrl=0x%08x;l3=0x%08x;module=(want0)0x%08x\r\n", (unsigned)ctrl, (unsigned)l3, (unsigned)module );
  // >>>> ctrl: 0x0a = 1010 = 2,2 : manual says 2 is not a valid value! cv_5v4 2015.11.02 p. 17-64.
  alt_write_word( SYSMGR_EMAC_CTRL, 0x5 );
  // Now we need to reset the module! Enter reset (1) wait leave reset (0):
  fprintf(stderr,"PERMODRST:0x%08x\r\n",(unsigned)alt_read_word(RSTMGR_PERMODRST));
  alt_write_word( RSTMGR_PERMODRST, alt_read_word(RSTMGR_PERMODRST) | (1U<<EMAC_CFG_NUM) );
  for(int i=0;i<1;++i) {
    fprintf(stderr,"PERMODRST:0x%08x\r\n",(unsigned)alt_read_word(RSTMGR_PERMODRST));
    // Make the EMAC cache-coherent - will only help if you also set SMP in SCU...???TODO/TBC
    alt_write_word( SYSMGR_EMAC_L3, 0xffffffffU );
  }
  alt_write_word( RSTMGR_PERMODRST, alt_read_word(RSTMGR_PERMODRST) & (~(1U<<EMAC_CFG_NUM)) );
  fprintf(stderr,"PERMODRST:0x%08x\r\n",(unsigned)alt_read_word(RSTMGR_PERMODRST));
  //
  fprintf(stderr,"sysmgr:ctrl=0x%08x\r\n", (unsigned)alt_read_word(SYSMGR_EMAC_CTRL) );
  //


  alt_freq_t freq=0;
  ok =( ALT_E_SUCCESS == alt_clk_freq_get( EMAC_CFG_CLK, &freq ) );
  ok = ok && (250000000U == freq);

  fprintf(stderr,"PERMODRST:0x%08x\r\n",(unsigned)alt_read_word(RSTMGR_PERMODRST));
  fprintf(stderr,"CLK:%c:%u\r\n",(ok?'1':'0'),(unsigned)freq);
  fprintf(stderr,"SysMgr:EMAC:Ctl:0x%08x\r\n",(unsigned)alt_read_word(ALT_SYSMGR_EMAC_ADDR) );
  fprintf(stderr,"SysMgr:EMAC:L3:0x%08x\r\n",(unsigned)alt_read_word(ALT_SYSMGR_EMAC_ADDR+4) );


  return ok;
}

