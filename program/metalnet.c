
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "io.h"
#include "emac_core.h"
#include "eapp.h"
#include "sctlr.h"
#include "actlr.h"

#include "alt_globaltmr.h"
#include "alt_timers.h"
#include "alt_interrupt.h"
#include "alt_cache.h"

#include "scu.h"

#include "alt_pt.h"
#include "alt_address_space.h"


#include "eip.h"
#include "baremetal.h"

#include "emac_phy_micrel9031.h"

static uint64_t ts = 0;

void startTimer() {
  ts = alt_globaltmr_get64();
}

void stopTimerReport() {
  const uint64_t delta = alt_globaltmr_get64() - ts;
  fprintf(stderr,"dT=%08x%08x/%08x\r\n", (unsigned)(delta>>32), (unsigned)delta, (unsigned)alt_gpt_freq_get(ALT_GPT_CPU_GLOBAL_TMR) );
}

void isr_handler(uint32_t icciar, void *context) {
  alt_int_dist_pending_clear( EMAC_INTERRUPT );
  emac_int_handle();
  alt_int_dist_pending_clear( EMAC_INTERRUPT );
  printf("[ISR]\r\n");
}
void tick_handler(uint32_t icciar, void *context) {
  alt_gpt_int_clear_pending( ALT_GPT_CPU_PRIVATE_TMR );
  alt_int_dist_pending_clear( ALT_INT_INTERRUPT_PPI_TIMER_PRIVATE );
  printf("[tick%d]\r\n",io_input_available(0));
}

#ifndef ALT_INT_PROVISION_VECTOR_SUPPORT
#error Compile all with interrupt support
#endif

static bool sys_init(void)
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
    fprintf(stderr,"\r\n");
    fprintf(stderr,"INFO: System Initialization.\r\n");
    // Initialize global timer
    if(status == ALT_E_SUCCESS) {
        fprintf(stderr,"INFO: Setting up Global Timer.\r\n");
        if(!alt_globaltmr_int_is_enabled()) {
            status = alt_globaltmr_init();
        }
    }
    // Initialize general purpose timers
    if(status == ALT_E_SUCCESS) {
        fprintf(stderr,"INFO: Initializing General Purpose Timers.\r\n");
        status = alt_gpt_all_tmr_init();
    }
    bool ok = (ALT_E_SUCCESS==status);
    fprintf(stderr,"INFO: Init:%c\r\n",ok?'y':'N');
    return ok;
}

static bool soc_int_setup()
{
    ALT_STATUS_CODE status = ALT_E_SUCCESS;
//    int cpu_target = 0x1; //CPU0 will handle the interrupts
    fprintf(stderr,"\r\n");
    fprintf(stderr,"INFO: Interrupt Setup.\r\n");
    // Initialize global interrupts
    if (status == ALT_E_SUCCESS) {
        status = alt_int_global_init();
    }
    // Initialize CPU interrupts
    if (status == ALT_E_SUCCESS) {
        status = alt_int_cpu_init();
    }
//----
    if (status == ALT_E_SUCCESS) {
        status = alt_int_dist_trigger_set(ALT_INT_INTERRUPT_PPI_TIMER_PRIVATE, ALT_INT_TRIGGER_AUTODETECT);
    }
    if (status == ALT_E_SUCCESS) {
        status = alt_int_dist_enable(ALT_INT_INTERRUPT_PPI_TIMER_PRIVATE);
    }
//----
    // Enable CPU interrupts
    if (status == ALT_E_SUCCESS) {
        status = alt_int_cpu_enable();
    }
    // Enable global interrupts
    if (status == ALT_E_SUCCESS) {
        status = alt_int_global_enable();
    }
    bool ok = (ALT_E_SUCCESS==status);
    fprintf(stderr,"INFO: SoC-Init:%c\r\n",ok?'y':'N');
    return ok;
}

int main(void)
{
    alt_pt_uninit();
    io_init();
    bool ok = sys_init();
    ok &= soc_int_setup();

    bool enok = true;
    enok &=( ALT_E_SUCCESS == alt_int_isr_register( EMAC_INTERRUPT, isr_handler, 0 ) );
    enok &=( ALT_E_SUCCESS == alt_int_dist_target_set( EMAC_INTERRUPT, 1<<0 ) );
    enok &=( ALT_E_SUCCESS == alt_int_dist_trigger_set( EMAC_INTERRUPT, ALT_INT_TRIGGER_AUTODETECT ) ); //...LEVEL ) );
    enok &=( ALT_E_SUCCESS == alt_int_dist_enable( EMAC_INTERRUPT ) );
    fprintf(stderr,">>>IntEn:%d\r\n",enok?1:0);


    const uint32_t freq = alt_gpt_freq_get(ALT_GPT_CPU_PRIVATE_TMR);
    fprintf(stderr,"freq=%uHz\r\n",(unsigned)freq);
    alt_gpt_mode_set(ALT_GPT_CPU_PRIVATE_TMR, ALT_GPT_RESTART_MODE_PERIODIC);
    alt_gpt_counter_set( ALT_GPT_CPU_PRIVATE_TMR, freq );
    alt_int_isr_register( ALT_INT_INTERRUPT_PPI_TIMER_PRIVATE, tick_handler, 0 );
    alt_gpt_int_enable(ALT_GPT_CPU_PRIVATE_TMR);
    alt_gpt_tmr_start(ALT_GPT_CPU_PRIVATE_TMR);


    for(int i=0;i<5;++i) fprintf(stderr,"ticks=%u\r\n",(unsigned)alt_gpt_counter_get(ALT_GPT_CPU_PRIVATE_TMR) );
    while( alt_gpt_counter_get(ALT_GPT_CPU_PRIVATE_TMR) > (freq>>1)) { } //NULL

    ok &= (ALT_E_SUCCESS==alt_pt_init());
    // NOTE: Booting the cache means managing coherency ... TODO!!!!
    ok &= (ALT_E_SUCCESS==alt_cache_system_enable());
    //// Is coherence just an L1D issue??? NO.
    //ok &= (ALT_E_SUCCESS==alt_cache_l1_data_disable());
    char xxx[5];
//    io_init();
    // ---- Text I/O allowed now ----
/*** This turns on SMP and so we should get coherency ... BUT WE DONT!!!!!! TODO
    fprintf(stderr,"SCU:CTL:%08x;CFG:%08x\r\n",(unsigned)scu_getCtl(),(unsigned)scu_getCfg() );
    scu_setCtl( scu_getCtl() | 0x9U ); // 1=SCU on; 8=speculative-fills
    fprintf(stderr,"SCU:CTL:%08x;CFG:%08x\r\n",(unsigned)scu_getCtl(),(unsigned)scu_getCfg() );
    scu_secure_invalidate( 0, 0xfU ); // CPU0, all ways.
    fprintf(stderr,"ACTLR:%08x\r\n", (unsigned)get_actlr() );
    set_actlr( get_actlr() | 0x0045 ); // 0x40=SMP; 4=???; 1=MaintBdcst
    fprintf(stderr,"ACTLR:%08x\r\n", (unsigned)get_actlr() );



// TODO: Maybe also need this kind of thing:
ALT_STATUS_CODE alt_acp_id_map_fixed_read_set(const uint32_t input_id,
                                              const uint32_t output_id,
                                              const ALT_ACP_ID_MAP_PAGE_t page,
                                              const uint32_t aruser)
ALT_STATUS_CODE alt_acp_id_map_fixed_write_set(const uint32_t input_id,
                                               const uint32_t output_id,
                                               const ALT_ACP_ID_MAP_PAGE_t page,
                                               const uint32_t awuser)
// Input Id: ALT_ACP_ID_MAP_MASTER_ID_EMAC1(var)   (0x00000802 | (0x00000878 & (var))) TODO: what is var???
// Output id: (fixed= 3..6); (dynamic= 3..7).
//   ALT_ACP_ID_OUT_DYNAM_ID_3 .. 6
// Page: ALT_ACP_ID_MAP_PAGE_0



***/
    const bool rdSetOk = (ALT_E_SUCCESS == alt_acp_id_map_dynamic_read_options_set( ALT_ACP_ID_MAP_PAGE_0, 31 ) );
    const bool wrSetOk = (ALT_E_SUCCESS == alt_acp_id_map_dynamic_write_options_set( ALT_ACP_ID_MAP_PAGE_0, 31 ) );
    fprintf(stderr,"ACPIDdynamic=%d,%d\r\n",rdSetOk?1:0,wrSetOk?1:0);
////
    fprintf(stderr,"SCU:CTL:%08x;CFG:%08x\r\n",(unsigned)scu_getCtl(),(unsigned)scu_getCfg() );
    startTimer();
    volatile int x=0;
    for(int i=0;i<100000;++i) x+= i;
    stopTimerReport();
    fprintf(stderr,"Hello:%c\r\n",ok?'y':'n');
    fprintf(stderr,"SCTLR:%08x\r\n", (unsigned)get_sctlr() );
    // Turn off SCTLR.A:
    set_sctlr( get_sctlr() & (~2U) );
    // NOTE: WITH SCTLR.A=0 ALONE, STILL GET ALIGNMENT FAULTS 
    // - WE ALSO NEED THE *PT* SET UP.
    fprintf(stderr,"SCTLR:%08x\r\n", (unsigned)get_sctlr() );
    *((int*)(&xxx[0])) += 1;
    *((int*)(&xxx[1])) += 1;
    // 
/**
    bool enok = true;
    enok &=( ALT_E_SUCCESS == alt_int_global_init() );
    enok &=( ALT_E_SUCCESS == alt_int_global_enable_all() );
    enok &=( ALT_E_SUCCESS == alt_int_cpu_enable() );
    enok &=( ALT_E_SUCCESS == alt_int_isr_register( EMAC_INTERRUPT, isr_handler, 0 ) );
    enok &=( ALT_E_SUCCESS == alt_int_dist_trigger_set( EMAC_INTERRUPT, ALT_INT_TRIGGER_AUTODETECT ) ); //...LEVEL ) );
    enok &=( ALT_E_SUCCESS == alt_int_dist_enable( EMAC_INTERRUPT ) );
    enok &=( ALT_E_SUCCESS == alt_int_dist_secure_enable( EMAC_INTERRUPT ) );
    fprintf(stderr,">>>IntEn:%d\r\n",enok?1:0);
**/
    // 
    eap_init(emac_phy_init_micrel9031_atlas);
    while(true) {
      emac_rx_service();
      if(io_input_available(0)) {
        static char BM_CACHE_ALIGN msg[] = "This is a message long enough to not be considered a runt i.e. >64 total length ... =[_]";
        const uint16_t msglen = (uint16_t)strlen(msg);
        char c='?';
        if(1!=read(0,&c,1)) c = '!';
        msg[msglen-2] = c;
        eip_txpkt_udp_zc((uint8_t*)msg, msglen, eap_getPDstUdpId());
        printf("[%c]\r\n",c);
      }
    }
    return 0;
}

