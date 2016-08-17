

#include <stdint.h>
#include <stdbool.h>

#include <stdio.h>

#include "socal/socal.h"


#include "alt_interrupt.h"

#include "baremetal.h"
#include "ctassert.h"

// Customisation - our MAC address.
#include "emac_cfg.h"

#include "ephy.h"


#include "emac_core.h"

// This is the core module for the Synopsys Designware MAC.
// It has been written from scratch using the Altera Cyclone V SoC HPS documentation as reference
// and with it (bare metal) as intended target.
// Nonetheless the design consciously attempts to minimise dependencies and ease porting.

/****
* From CV Tech Ref Manual, Chapter 17 EMAC:

EMAC HPS Interface Initialization
To initialize the Ethernet controller to use the HPS interface, specific software steps must be followed
including selecting the correct PHY interface through the System Manager.
In general, the Ethernet Controller must be in a reset state during static configuration and the clock must
be active and valid before the Ethernet Controller is brought out of reset.
1. After the HPS is released from cold or warm reset, reset the Ethernet Controller module by setting the
appropriate emac bit in the permodrst register in the Reset Manager.
2. Configure the EMAC Controller clock to 250 MHz by programming the appropriate cnt value in the
emac*clk register in the Clock Manager.
3. Bring the Ethernet PHY out of reset to verify that there are RX PHY clocks.
4. When all the clocks are valid, program the following clock settings:
a. Set the physel_* field in the ctrl register of the System Manager (EMAC Group) to 0x1 to select
the RGMII PHY interface.
b. Disable the Ethernet Controller FPGA interfaces by clearing the emac_* bit in the module register
of the System Manager (FPGA Interface group).
5. Configure all of the EMAC static settings if the user requires a different setting from the default value.
These settings include AXI AxCache signal values, which are programmed in l3 register in the
EMAC group of the System Manager.
6. Execute a register read back to confirm the clock and static configuration settings are valid.
7. After confirming the settings are valid, software can clear the emac bit in the permodrst register of the
Reset Manager to bring the EMAC out of reset..
When these steps are completed, general Ethernet controller and DMA software initialization and
configuration can continue.

****/


// Real-Time Use
// Initialisation is non-real-time.
// Access to the PHY waits the response and so is quick but blocking. TODO: That could be fixed?
// However PHY access post-initialisation only applies on link state changes.


// We can see experimentally that:
// - Preamble + SFD (first 8 bytes of Ethernet layer) are not included.
// - We start with dstMAC; srcMAC; (VLAN optional); Ethertype: 14+ bytes
// - Normal packet payload is 46..1500 bytes (min =42 when VLAN tagged)
// - FCS 4bytes

// ----
// For blinkered UDP/TCP/IP:
// - set bit 20+21 in the MAC filter.
// - Set L3_LT_Control0..3: src/dst port filter; udp/tcp filter;
//   src/dst addr filter-mask-size; src/dst addr filter.
// - - Set L4_Addr0..3_Reg0 if src/dst port filtering.
// - - Set L3_Addr1_Reg1 to IP addr
// Static ARP entry on the host - non UDP/TCP ar filtered out.
// ----







// Descriptors are normally 4x32bit but can be enhanced and 8x32bit.
// NOTE: DMA Bus_Mode register bit 7 atds sets the size 0:4;1:8.
// TODO: TBC: It may be appropriate to enforce enhanced descriptors and their equivalence to cache-line size.
#define EMAC_DESC_IS_ENHANCED  1U  /* 1(true) or 0(false) */
#define EMAC_DESC_SIZE32       (4U<<EMAC_DESC_IS_ENHANCED)
#define EMAC_DESC_SIZE8        (EMAC_DESC_SIZE32<<2)
#define EMAC_NUM_TX            4U
#define EMAC_NUM_RX            4U
//
#define EMAC_DESC_PER_CACHE_LINE   (BM_CACHE_LINE/EMAC_DESC_SIZE8)
// There are a whole number of descriptors in a cache line.
CTASSERT_SIZE_ISCACHELINES( EMAC_DESC_SIZE8*EMAC_DESC_PER_CACHE_LINE, emacDescPerCacheLine );
CTASSERT_ISPOW2(EMAC_DESC_PER_CACHE_LINE, emacDescPerCacheLinePow2 );
//
// 1536=3x512. The max normal Ethernet payload size is 1500bytes.
// Preamble+SFD is n/a for us, so we have: 14+ bytes in front and FCS=4bytes
// behind. (>14 if VLAN tagged) So this size accommodates normal packets that
// are not madly recursively VLAN encapsulated.
#define EMAC_FRAGSIZE       1536U

// Tx Descriptor word32 0:
#define EMAC_TXDESC0_OWN        0x80000000U
#define EMAC_TXDESC0_IC         0x40000000U
#define EMAC_TXDESC0_LS         0x20000000U
#define EMAC_TXDESC0_FS         0x10000000U
#define EMAC_TXDESC0_DC         0x08000000U
#define EMAC_TXDESC0_DP         0x04000000U
#define EMAC_TXDESC0_TTSE       0x02000000U
#define EMAC_TXDESC0_CIC_NONE   0x00000000U
#define EMAC_TXDESC0_CIC_IP     0x00400000U
#define EMAC_TXDESC0_CIC_IP_PAY 0x00800000U
#define EMAC_TXDESC0_CIC_ALL    0x00c00000U
#define EMAC_TXDESC0_TER        0x00200000U
#define EMAC_TXDESC0_TCH        0x00100000U
#define EMAC_TXDESC0_TTSS       0x00020000U
#define EMAC_TXDESC0_IHE        0x00010000U
#define EMAC_TXDESC0_ES         0x00008000U
#define EMAC_TXDESC0_JT         0x00004000U
#define EMAC_TXDESC0_FF         0x00002000U
#define EMAC_TXDESC0_IPE        0x00001000U
#define EMAC_TXDESC0_LateC      0x00000800U
#define EMAC_TXDESC0_NC         0x00000400U
#define EMAC_TXDESC0_LossC      0x00000200U
#define EMAC_TXDESC0_EC         0x00000100U
#define EMAC_TXDESC0_VF         0x00000080U
#define EMAC_TXDESC0_CC_MASK    0x00000078U
#define EMAC_TXDESC0_ED         0x00000004U
#define EMAC_TXDESC0_UF         0x00000002U
#define EMAC_TXDESC0_DB         0x00000001U

// Rx Descriptor word32 0:
#define EMAC_RXDESC0_OWN        0x80000000U
#define EMAC_RXDESC0_AFM        0x40000000U
#define EMAC_RXDESC0_FL_MASK    0x3fff0000U
#define EMAC_RXDESC0_ES         0x00008000U /*ErrorSummary:DE|OE|GF|LC|WT|RE|CE|xxx*/
#define EMAC_RXDESC0_DE         0x00004000U
#define EMAC_RXDESC0_SAF        0x00002000U
#define EMAC_RXDESC0_LE         0x00001000U
#define EMAC_RXDESC0_OE         0x00000800U
#define EMAC_RXDESC0_VLAN       0x00000400U
#define EMAC_RXDESC0_FS         0x00000200U
#define EMAC_RXDESC0_LD         0x00000100U
#define EMAC_RXDESC0_XX         0x00000080U
#define EMAC_RXDESC0_LC         0x00000040U
#define EMAC_RXDESC0_FT         0x00000020U
#define EMAC_RXDESC0_RWT        0x00000010U
#define EMAC_RXDESC0_RE         0x00000008U
#define EMAC_RXDESC0_DribE      0x00000004U
#define EMAC_RXDESC0_CE         0x00000002U
#define EMAC_RXDESC0_ESA_RMA    0x00000001U
// Rx Descriptor word32 1:
#define EMAC_RXDESC1_DIC        0x80000000U
#define EMAC_RXDESC1_RBS2_MASK  0x1fff0000U
#define EMAC_RXDESC1_RER        0x00008000U
#define EMAC_RXDESC1_RCH        0x00004000U
#define EMAC_RXDESC1_RBS1_MASK  0x00001fffU

// Form Tx/Rx desc word 1 from a pair of buffer sizes.
#define MK_DESCSIZES(ONE,TWO) (((TWO&0x1fffU)<<16)|(ONE&0x1fffU))

static t_rxCallBack rxCallBack = 0;

// Packet Buffers:
static char BM_CACHE_ALIGN rbuf[EMAC_FRAGSIZE*EMAC_NUM_RX];
static char BM_CACHE_ALIGN tbuf[EMAC_FRAGSIZE*EMAC_NUM_TX];
// Each buffer in their spaces is 'lines' and so are the overall buffer spaces.
CTASSERT_SIZE_ISCACHELINES( EMAC_FRAGSIZE, emacFragSize );
CTASSERT_DECL_ISCACHELINES( rbuf, rbufSize );
CTASSERT_DECL_ISCACHELINES( tbuf, tbufSize );


// Descriptor Buffers:
static uint32_t BM_CACHE_ALIGN txdesc[EMAC_DESC_SIZE32*EMAC_NUM_TX];
static uint32_t BM_CACHE_ALIGN rxdesc[EMAC_DESC_SIZE32*EMAC_NUM_RX];
//
CTASSERT_DECL_ISCACHELINES( txdesc, txdescSize );
CTASSERT_DECL_ISCACHELINES( rxdesc, rxdescSize );



/// Where to look in txdesc for the next available tx descriptor.
static uint32_t txdescidx=0;
/// Where to look in rxdesc for the next used rx descriptor.
static uint32_t rxdescidx=0;
//
/// Initialise the Tx descriptors to point to the Tx buffers - but nothing to send.
/// We therefore retain ownership.
static void init_txdesc() {
  for(int i=0;i<EMAC_NUM_TX;++i) {
    uint8_t *baseAddr = (uint8_t*)&tbuf[i*EMAC_FRAGSIZE];
    //
    txdesc[i*EMAC_DESC_SIZE32+0] = EMAC_TXDESC0_LS | EMAC_TXDESC0_FS | EMAC_TXDESC0_CIC_ALL;
    // Note: if only using one buffer it must be the 2nd that is non-zero!
    txdesc[i*EMAC_DESC_SIZE32+1] = (uint32_t)0;
    txdesc[i*EMAC_DESC_SIZE32+2] = (uint32_t)0;
    txdesc[i*EMAC_DESC_SIZE32+3] = (uint32_t)baseAddr;
    //
  }
  // Last entry terminates ring.
  txdesc[(EMAC_NUM_TX-1)*EMAC_DESC_SIZE32] |= EMAC_TXDESC0_TER;

  //
  BM_CACHE_PUSH( txdesc, sizeof(txdesc) );
  BM_CACHE_PUSHOUT( tbuf, sizeof(tbuf) );
  //
  txdescidx=0;
}

/// Initialise the Rx descriptors to point to the Rx buffers - ready to receive.
/// We therefore give DMA ownership.
static void init_rxdesc() {
  for(int i=0;i<EMAC_NUM_RX;++i) {
    rxdesc[i*EMAC_DESC_SIZE32+0] = EMAC_RXDESC0_OWN;
    rxdesc[i*EMAC_DESC_SIZE32+1] = EMAC_FRAGSIZE<<16; //|EMAC_RXDESC1_RCH;
    rxdesc[i*EMAC_DESC_SIZE32+2] = (uint32_t)0;
    rxdesc[i*EMAC_DESC_SIZE32+3] = (uint32_t)&rbuf[i*EMAC_FRAGSIZE];
  }
  // Last entry terminates ring.
  rxdesc[(EMAC_NUM_RX-1)*EMAC_DESC_SIZE32+1] |= EMAC_RXDESC1_RER;
  //
  BM_CACHE_PUSH( rxdesc, sizeof(rxdesc) );
  BM_CACHE_INVALIDATE( rbuf, sizeof(rbuf) );
  //
  rxdescidx=0;
}

/// Base address of EMAC periperals.
#define DWC_EMAC_ADDRBASE   0xff700000
/// Size of the address space of an EMAC peripheral.
#define DWC_EMAC_ADDRSIZE   0x00002000
/// The EMAC peripheral we are targetting - config setting.
#define DWC_EMAC_BASE       (DWC_EMAC_ADDRBASE+(DWC_EMAC_ADDRSIZE*EMAC_CFG_NUM))

CTASSERT((EMAC_CFG_NUM>=0)&&(EMAC_CFG_NUM<=1),emac_num_0_or_1);

#define DWC_EMAC_MACCONFIG  (DWC_EMAC_BASE+0x0000)
#define DWC_EMAC_MACFILTER  (DWC_EMAC_BASE+0x0004)
#define DWC_EMAC_GMIIADDR   (DWC_EMAC_BASE+0x0010)
#define DWC_EMAC_GMIIDATA   (DWC_EMAC_BASE+0x0014)
#define DWC_EMAC_FLOWCTRL   (DWC_EMAC_BASE+0x0018)

#define DWC_EMAC_DEBUG      (DWC_EMAC_BASE+0x0024)

#define DWC_EMAC_INTSTS     (DWC_EMAC_BASE+0x0038)
#define DWC_EMAC_INTMSK     (DWC_EMAC_BASE+0x003c)


#define DWC_EMAC_MACHI0     (DWC_EMAC_BASE+0x0040)
#define DWC_EMAC_MACLO0     (DWC_EMAC_BASE+0x0044)

#define DWC_EMAC_MACHI16    (DWC_EMAC_BASE+0x0800)

#define DWC_EMAC_CTLSTS     (DWC_EMAC_BASE+0x00d8)

#define DWC_EMAC_RXFRAME    (DWC_EMAC_BASE+0x0180)
#define DWC_EMAC_RXBYTE     (DWC_EMAC_BASE+0x0184)
#define DWC_EMAC_RXBYTEOK   (DWC_EMAC_BASE+0x0188)

// DWC_EMAC_MACCONFIG[15:14]
#define DWC_EMAC_MACCONFIG_10M             (2U<<14)
#define DWC_EMAC_MACCONFIG_100M            (3U<<14)
#define DWC_EMAC_MACCONFIG_1000M           0
// Other bits - only the interesting ones defined:
#define DWC_EMAC_MACCONFIG_2KFRAMEOK       (1U<<27)
#define DWC_EMAC_MACCONFIG_CST             (1U<<25)
#define DWC_EMAC_MACCONFIG_TXPHY           (1U<<24)
#define DWC_EMAC_MACCONFIG_BURST           (1U<<21)
#define DWC_EMAC_MACCONFIG_JUMBOFRAMEOK    (1U<<20)
#define DWC_EMAC_MACCONFIG_LOOPBACK        (1U<<12)
#define DWC_EMAC_MACCONFIG_MACFULLDUPLEX   (1U<<11)
#define DWC_EMAC_MACCONFIG_CHKSUM          (1U<<10)
#define DWC_EMAC_MACCONFIG_LINKUP          (1U<<8)
/* #define DWC_EMAC_MACCONFIG_ACS             (1U<<7) not useful */
#define DWC_EMAC_MACCONFIG_TXENABLE        (1U<<3)
#define DWC_EMAC_MACCONFIG_RXENABLE        (1U<<2)


//----
#define DWC_DMA_BASE        (DWC_EMAC_BASE+0x1000U)
//
#define DWC_DMA_BUSMODE     (DWC_DMA_BASE+0x0000)
#define DWC_DMA_TXPOLL      (DWC_DMA_BASE+0x0004)
#define DWC_DMA_RXPOLL      (DWC_DMA_BASE+0x0008)
#define DWC_DMA_RXDESC      (DWC_DMA_BASE+0x000c)
#define DWC_DMA_TXDESC      (DWC_DMA_BASE+0x0010)
#define DWC_DMA_STATUS      (DWC_DMA_BASE+0x0014)
#define DWC_DMA_OPMODE      (DWC_DMA_BASE+0x0018)
#define DWC_DMA_INTEN       (DWC_DMA_BASE+0x001c)
#define DWC_DMA_MISSOVF     (DWC_DMA_BASE+0x0020)
#define DWC_DMA_INTWDOG     (DWC_DMA_BASE+0x0024)
#define DWC_DMA_AXIMODE     (DWC_DMA_BASE+0x0028)
#define DWC_DMA_AXISTATUS   (DWC_DMA_BASE+0x002c)
#define DWC_DMA_CURRTXDESC  (DWC_DMA_BASE+0x0048)
#define DWC_DMA_CURRRXDESC  (DWC_DMA_BASE+0x004c)
#define DWC_DMA_CURRTXBUF   (DWC_DMA_BASE+0x0050)
#define DWC_DMA_CURRRXBUF   (DWC_DMA_BASE+0x0054)
#define DWC_DMA_HWFEATURE   (DWC_DMA_BASE+0x0058)




/// Force the DMA to re-evaluate the Tx descriptor list for something to tx.
static void dma_txpoll() {
  alt_write_word( DWC_DMA_TXPOLL, 0 );
}

/// Force the DMA to re-evaluate the Rx descriptor list for space to rx.
static void dma_rxpoll() {
  alt_write_word( DWC_DMA_RXPOLL, 0 );
}



// Get a txdesc, assume it will be used.
// (Caller should use each one as it gets it and not get multiple before using any.)
static uint32_t *get_free_txdesc() {
  uint32_t *desc = &txdesc[EMAC_DESC_SIZE32*txdescidx];
  // Refresh the current idx line.
  BM_CACHE_INVALIDATE( (uint32_t*)(((uint32_t)desc)&(~(BM_CACHE_LINE-1U))), BM_CACHE_LINE );
  // If DMA owns the current idx then bad return, else assume it'll be used and advance.
  if(*desc & EMAC_TXDESC0_OWN)
    desc=0;
  else
    txdescidx = (txdescidx+1) & (EMAC_NUM_TX-1);
  printf("txdesc=%08x\r\n",(unsigned)desc);
  return desc;
}

// The desc_sizes are the correctly combined size of BOTH descriptors using MKDESCSIZES.
static void give_txdesc( uint32_t *desc, uint32_t desc_sizes ) {
  desc[1] = desc_sizes;
  desc[0] |= EMAC_TXDESC0_OWN | EMAC_TXDESC0_IC;
  BM_CACHE_PUSH( desc, BM_CACHE_LINE ); //TODO
  dma_txpoll();
}

// ----
// TODO:
// It would be optimal to have a tx version that allowed hdr and body to be
// separate and body (if not also hdr) to be under caller control.
// Assuming the current way remains the norm then, after tx a clean up would be
// needed to restore order.
// Most optimal tx is:
// - Have a header set up once and only change the seq# and len each time.
// - First tx buffer points to header for length of header.
// - Second tx buffer points to body which can be anywhere but prob best cache
//   quantized.
// ----
// Attempt to allocate a Tx descriptor returning a handle to it and
// its associated buffer into which we put our packet.
// Note we may fail to have a Tx buffer available (e.g. if transmitting
// faster than the link) and return 0.
uint32_t* emac_tx_alloc(uint8_t** ppbuf) {
    uint32_t* pdesc = ppbuf?get_free_txdesc():0;
    uint8_t* pbuf = 0;
    if(pdesc) {
      // If pdesc[2] is non-zero then we assume the last use split hdr&bdy and
      // the hdr is our buffer and the bdy is the last user's.
      if(pdesc[2]) {
        pdesc[3] = pdesc[2];
        pdesc[2] = 0;
      }
      pbuf = (uint8_t*)pdesc[3];
    }
    *ppbuf = pbuf;
    return pdesc;
}
// Complete a previous allocation by requesting transmission.
void emac_tx_complete(uint32_t *pdesc, uint32_t buf_used) {
  if(pdesc&&buf_used) {
    // Only the 2nd desc is valid.
    uint8_t *pbuf = (uint8_t*)pdesc[3];
    BM_CACHE_PUSH( (void*)pbuf, EMAC_FRAGSIZE );
    give_txdesc( pdesc, MK_DESCSIZES(0,buf_used) );
  }
}
// TODO: how does caller know when tx complete and they can trash their bdy???
// ??? The Tx int tells us the pkt went. How does that convey release of caller
// ??? bdy? Could provision a gloabl tx-done callback which takes as param
// ??? bdy completed?
void emac_tx_complete_with_bdy(uint32_t *pdesc, uint32_t hdr_used, uint8_t* pbdy, uint32_t bdy_used) {
  if(pdesc&&hdr_used&&pbdy&&bdy_used) {
    // Shuffle our own (hdr) buffer into first place and put caller's bdy second.
    pdesc[2] = pdesc[3];
    pdesc[3] = (uint32_t)pbdy;
    BM_CACHE_PUSH( (void*)pdesc[2], EMAC_FRAGSIZE );
    BM_CACHE_PUSH( (void*)pdesc[3], EMAC_FRAGSIZE );
    give_txdesc( pdesc, MK_DESCSIZES(hdr_used,bdy_used) );
  }
}



// ----
// For rxdesc...
// - we start (init) to have given all EMAC_NUM_RX, and to have received 0.
// - if rx state-machine halted then all returned.
// - (otherwise) DWC_DMA_CURRRXDESC can tell us how many rx'ed
// - (or) we can proceed round-robin consuming all that is ours and then giving
//   desc (kick rx machine if halted).
// ----
// NOTE:TODO: With caching it seems we need to only give a whole lineful at a time
// or else we may push back dirty over a concurrent dma mod.
static void rxdesc_process() {
  uint32_t *desc = &rxdesc[EMAC_DESC_SIZE32*rxdescidx];
  uint32_t lineidx = rxdescidx&(~(EMAC_DESC_PER_CACHE_LINE-1U));
  uint32_t *line = &rxdesc[EMAC_DESC_SIZE32*lineidx];
  // Invalidate the initial line.
  BM_CACHE_INVALIDATE( line, BM_CACHE_LINE );
  // Process until 
  while(0==( *desc & EMAC_RXDESC0_OWN )) {
    if(0!=( *desc & EMAC_RXDESC0_ES ))
      printf("rxERR:%x\r\n",(unsigned)*desc);
    emac_int_decode(); //TODO
    // Call-out to consume this entry's packet.
    if(rxCallBack) {
      uint8_t* pPkt = (uint8_t*)desc[3];
      BM_CACHE_INVALIDATE( pPkt, EMAC_FRAGSIZE ); //TODO: scale length
      rxCallBack( pPkt, desc[1]>>16 );
    }
    // Next:
    rxdescidx = (rxdescidx+1) & (EMAC_NUM_RX-1);
    // Have we crossed a cache line?
    const uint32_t nextlineidx = rxdescidx&(~(EMAC_DESC_PER_CACHE_LINE-1U));
    if(nextlineidx!=lineidx) {
      // Give all descriptors in the last line.
      for(uint32_t i=lineidx;i<lineidx+EMAC_DESC_PER_CACHE_LINE;++i) {
        rxdesc[EMAC_DESC_SIZE32*i] = EMAC_RXDESC0_OWN;
      }
      // Clean the modified last line.
      line = &rxdesc[EMAC_DESC_SIZE32*lineidx];
      BM_CACHE_PUSH( line, BM_CACHE_LINE );
      lineidx = nextlineidx;
      // Invalidate the next line.
      line = &rxdesc[EMAC_DESC_SIZE32*lineidx];
      BM_CACHE_INVALIDATE( line, BM_CACHE_LINE );
      // Kick the rx state-machine speculatively in case it halted - out of rxdescriptors.
      dma_rxpoll();
    }
  }
}
// ----


void emac_rx_service() {
  dma_rxpoll();
  rxdesc_process();
}

// 
static bool emac_dma_init() {
  bool ok = true;

  fprintf(stderr,"DMA:BusMode:0x%08x\r\n",(unsigned)alt_read_word(DWC_DMA_BUSMODE) );

  // [DMA 1,2] Reset the BusMode, await 0 lsb; prog burst etc.
  alt_write_word( DWC_DMA_BUSMODE, 1 );
  while( 1&alt_read_word(DWC_DMA_BUSMODE) ) { }
  //
  alt_write_word( DWC_DMA_BUSMODE, /*alt_read_word(DWC_DMA_BUSMODE) |*/ (EMAC_DESC_IS_ENHANCED<<7) | (1U<<16)|(3U<<14)|(16U<<8) );


  // [DMA 5] AXI Mode
  alt_write_word( DWC_DMA_AXIMODE, 0xeU );

  // [DMA 6,7] Initialise Rx & Tx descriptors.
  init_txdesc();
  init_rxdesc();
  // [DMA 8] Set Rx & Tx Desc base addr.
  alt_write_word( DWC_DMA_RXDESC, (uint32_t)&rxdesc[0] );
  alt_write_word( DWC_DMA_TXDESC, (uint32_t)&txdesc[0] );

  // [DMA 13] Start Tx&Rx DMA: (bit21 only tx when fifo holds whole frame may be necessary to prevent underflow in batched sends when cpu uncached?)
  alt_write_word( DWC_DMA_OPMODE, (uint32_t)((1U<<26)|(1U<<25)|(1U<<7)|(1U<<6)|(1U<<21)) );
  alt_write_word( DWC_DMA_OPMODE, alt_read_word(DWC_DMA_OPMODE) | (1U<<1) );
  alt_write_word( DWC_DMA_OPMODE, alt_read_word(DWC_DMA_OPMODE) | (1U<<13) );

  // TODO:Here? IntEn
  alt_write_word( DWC_DMA_STATUS, 0x1ffffU & alt_read_word( DWC_DMA_STATUS ) );
  alt_write_word( DWC_DMA_INTEN, 0x0001a1ffU );

  fprintf(stderr,">>DMA:>INT:En=%08x;Sts=%08x\r\n",(unsigned)alt_read_word( DWC_DMA_INTEN ),(unsigned)alt_read_word( DWC_DMA_STATUS ) );
  return ok;
}

// ----
// PHY Register Routines

// Internal definitions - relating to MAC interface to PHY
#define EMAC_PHY_CLKDIV 0
#define EMAC_PHY_CONST  ( ((EMAC_CFG_PHY_ADDR)<<11) | ((EMAC_PHY_CLKDIV)<<2) )
#define EMAC_PHY_WRITE  2
#define EMAC_PHY_BUSY   1

// (Not really the PHY that is busy but rather the (R)(G)MII interface between
// EMAC and it.)
static bool emac_phyif_busy() {
  return (0 != (EMAC_PHY_BUSY & alt_read_word(DWC_EMAC_GMIIADDR)) );
}

/// Read a register back from the PHY.
static uint16_t emac_phy_read( uint8_t reg ) {
  while (emac_phyif_busy()) { }
  alt_write_word( DWC_EMAC_GMIIADDR, (EMAC_PHY_CONST | (reg<<6) | EMAC_PHY_BUSY) );
  while (emac_phyif_busy()) { }
  return( (uint16_t) alt_read_word(DWC_EMAC_GMIIDATA) );
}

/// Write to a register in the PHY.
static void emac_phy_write( uint8_t reg, uint16_t val ) {
  while (emac_phyif_busy()) { }
  alt_write_word( DWC_EMAC_GMIIDATA, val );
  alt_write_word( DWC_EMAC_GMIIADDR, (EMAC_PHY_CONST | (reg<<6) | EMAC_PHY_WRITE | EMAC_PHY_BUSY) );
  while (emac_phyif_busy()) { }
}

// Definitions relating to PHY.

/// Reset the PHY.
void emac_phy_reset() {
  // Assert reset within the PHY.
  emac_phy_write( 0, PHY_MASK(0,RESET) );
  // Await PHY clearing reset.
  while( emac_phy_read(0) & PHY_MASK(0,RESET) ) { } 
}

// In principle loopback routines use only IEEE standard registers and should
// work with any PHY.
// In practice the settings come from Micrel KSZ9031RNX manual p.29.

void emac_phy_loopback1000() {
  // (loopback; autoneg disabled; force 1000; full duplex)
  emac_phy_write(
    0,
    PHY_MASK(0,LOOPBACK) | PHY_R0_SPEED_1000 | PHY_MASK(0,FULL_DUPLEX)
  );
  // (enable master-slave manual; slave)
  emac_phy_write( 9, 1U<<12 );
}

void emac_phy_loopback100() {
  // (loopback; autoneg disabled; force 100; full duplex)
  emac_phy_write(
    0,
    PHY_MASK(0,LOOPBACK) | PHY_R0_SPEED_100 | PHY_MASK(0,FULL_DUPLEX)
  );
}

void emac_phy_loopback10() {
  // (loopback; autoneg disabled; force 10; full duplex)
  emac_phy_write(
    0,
    PHY_MASK(0,LOOPBACK) | PHY_R0_SPEED_10 | PHY_MASK(0,FULL_DUPLEX)
  );
}

// ----
// PHY MMD Register Read/Write
// Internal definitions:
// Writes to the CTRL reg select the MMD device 0..31 and the OPeration to do on it.
#define PHY_MMD_CTRL_REG   0x000dU
#define PHY_MMD_DATA_REG   0x000eU
// CTRL reg modifiers that select the operation to do.
#define PHY_MMD_OP_REGSEL  0x0000U
#define PHY_MMD_OP_DATA    0x4000U

/// Write to a MMD register in the PHY.
void emac_phy_mmd_write( uint8_t mmddev, uint16_t mmdreg, uint16_t mmdval ) {
  // Select the register to write.
  emac_phy_write( PHY_MMD_CTRL_REG, PHY_MMD_OP_REGSEL | mmddev );
  emac_phy_write( PHY_MMD_DATA_REG, mmdreg );
  // Provision the datum to be written.
  emac_phy_write( PHY_MMD_CTRL_REG, PHY_MMD_OP_DATA   | mmddev );
  emac_phy_write( PHY_MMD_DATA_REG, mmdval );
}

/// Read a MMD register from the PHY.
uint16_t emac_phy_mmd_read( uint8_t mmddev, uint16_t mmdreg ) {
  // Select the register to read.
  emac_phy_write( PHY_MMD_CTRL_REG, PHY_MMD_OP_REGSEL | mmddev );
  emac_phy_write( PHY_MMD_DATA_REG, mmdreg );
  // Access the datum to be read.
  emac_phy_write( PHY_MMD_CTRL_REG, PHY_MMD_OP_DATA   | mmddev );
  return emac_phy_read( PHY_MMD_DATA_REG );
}
// ----


// Internal datatype to succinctly capture the link partner status and capabilities.
#define PARTNER_LINKUP        (1U<<0)
#define PARTNER_AUTONEGD      (1U<<1)
#define PARTNER_1000M_FULL    (1U<<2)
#define PARTNER_1000M_HALF    (1U<<3)
#define PARTNER_100M_FULL     (1U<<4)
#define PARTNER_100M_HALF     (1U<<5)
#define PARTNER_10M_FULL      (1U<<6)
#define PARTNER_10M_HALF      (1U<<7)
typedef uint8_t tPartnerSts;
/// Get the status capabilities of the link partner according to the PHY.
/// (Note that there will be some delay incurred communicating with the PHY.)
/// The MAC CTLSTS reg can also tell us this info (via in-band signalling from
/// the PHY) but experiment suggests hysteresis in things like link speed i.e.
/// it may initially be incorrect and then presently come good.
tPartnerSts emac_phy_get_partner_sts() {
  tPartnerSts sts = (0!=(emac_phy_read(1)&PHY_MASK(1,LINK_UP))) ? PARTNER_LINKUP : 0;
  if(sts) {
    const uint32_t phyr10 = emac_phy_read(10);
    const uint32_t phyr5  = emac_phy_read(5);
    if( phyr10 & PHY_MASK(10,1000BASET_FULL) )
      sts |= PARTNER_1000M_FULL;
    if( phyr10 & PHY_MASK(10,1000BASET_HALF) )
      sts |= PARTNER_1000M_HALF;
    if( phyr5  & PHY_MASK(5,100BASETX_FULL) )
      sts |= PARTNER_100M_FULL;
    if( phyr5  & (PHY_MASK(5,100BASETX_HALF)|PHY_MASK(5,100BASET4_HALF)) )
      sts |= PARTNER_100M_HALF;
    if( phyr5  & PHY_MASK(5,10BASET_FULL) )
      sts |= PARTNER_10M_FULL;
    if( phyr5  & PHY_MASK(5,10BASET_HALF) )
      sts |= PARTNER_10M_HALF;
  }
  if(sts) fprintf(stderr,"PARTNER:%02x\r\n",sts);
  return sts;
}
/// Get the speed and duplex settings to apply in the MAC Config reg.
/// Use this on the sts if the link was auto-negotiated.
/// TODO: !!! the full suite of link speeds needs functional test inc half duplex!!!
uint32_t emac_cfg_speed_duplex( tPartnerSts sts ) {
  const uint32_t mac_mmi_speed =
    (sts & (PARTNER_1000M_FULL|PARTNER_1000M_HALF)) ? DWC_EMAC_MACCONFIG_1000M :
  ( (sts & (PARTNER_100M_FULL |PARTNER_100M_HALF) ) ? DWC_EMAC_MACCONFIG_100M  : DWC_EMAC_MACCONFIG_10M );
  const uint32_t mac_mmi_duplex =
    (sts &(PARTNER_1000M_FULL|PARTNER_100M_FULL|PARTNER_10M_FULL)) ? DWC_EMAC_MACCONFIG_MACFULLDUPLEX : 0;
  fprintf(stderr,"SPEED:%08x;DUPLEX=%08x\r\n",(unsigned)mac_mmi_speed,(unsigned)mac_mmi_duplex);
  return (mac_mmi_speed|mac_mmi_duplex);
}

static void emac_mac_config() {
  tPartnerSts partnerSts = emac_phy_get_partner_sts();
  // We seem to need to set the (G)MII speed to that which the PHY is up on.
  // TODO: This needs full test...
  if(PARTNER_LINKUP&partnerSts) {
    const uint32_t cfg_speed_duplex = emac_cfg_speed_duplex( partnerSts );
    alt_write_word( DWC_EMAC_MACCONFIG,
      DWC_EMAC_MACCONFIG_TXPHY
    | cfg_speed_duplex
    | DWC_EMAC_MACCONFIG_BURST
    | DWC_EMAC_MACCONFIG_LINKUP
    | DWC_EMAC_MACCONFIG_CST          //TODO:<<<Useful???
    | DWC_EMAC_MACCONFIG_TXENABLE
    | DWC_EMAC_MACCONFIG_RXENABLE
    );
    fprintf(stderr,"MACCfg:0x%08x\r\n", (unsigned)alt_read_word(DWC_EMAC_MACCONFIG) );
    fprintf(stderr,"Ctrl/Sts:0x%08x\r\n", (unsigned)alt_read_word(DWC_EMAC_CTLSTS) );
    emac_int_decode(); //TODO
  }
}

/// 
bool emac_is_link_up() {
  const uint32_t mask_linkup = 8U;
  const bool up =(0!=( mask_linkup & alt_read_word(DWC_EMAC_CTLSTS) ));
  return up;
}


#define EMAC_FILTER_RX_ALL              (1U<<31)
#define EMAC_FILTER_DROP_NON_TCP_UDP_L4 (1U<<21)
#define EMAC_FILTER_DROP_NON_L3_L4      (1U<<20)
#define EMAC_FILTER_DROP_NON_VLAN       (1U<<16)
#define EMAC_FILTER_PERFECT_OR_HASH     (1U<<10)
#define EMAC_FILTER_SA                  (1U<< 9)
#define EMAC_FILTER_SA_INV              (1U<< 8)
// TODO: PCF bit 6..7
#define EMAC_FILTER_BROADCAST           (1U<< 5)
#define EMAC_FILTER_MULTICAST_PASS      (1U<< 4)
#define EMAC_FILTER_DA_INV              (1U<< 3)
#define EMAC_FILTER_MULTICAST_HASH      (1U<< 2)
#define EMAC_FILTER_UNICAST_HASH        (1U<< 1)
#define EMAC_FILTER_PROMISCUOUS         (1U<< 0)
//
static void emac_set_filter( uint32_t val ) {
  alt_write_word( DWC_EMAC_MACFILTER, val );
}


// NOTE: The first byte on the wire is compared against lo LSByte.
static void emac_set_mac0( const uint8_t* pmac ) {
  if(pmac) {
    const uint32_t hi = (1U<<31)|(pmac[5]<<8)|pmac[4];
    const uint32_t lo = (pmac[3]<<24)|(pmac[2]<<16)|(pmac[1]<<8)|pmac[0];
    alt_write_word( DWC_EMAC_MACHI0, hi );
    alt_write_word( DWC_EMAC_MACLO0, lo );
  }
}

// Note MAC0 is reserved for self address, mandatory and special so not allowed
// here.
void emac_set_mac_filter( uint8_t id, tMacFilterMask mask, const uint8_t* pmac ) {
  if(pmac&&(id>0)&&(id<128)) {
    uint32_t addrhi = DWC_EMAC_MACHI0+(id<<3);
    const uint32_t hi = (mask<<24)|(pmac[5]<<8)|pmac[4];
    const uint32_t lo = (pmac[3]<<24)|(pmac[2]<<16)|(pmac[1]<<8)|pmac[0];
    // There is a gap between MAC0..15 and MAC16..127 so we need to add
    // the difference in base addresses but stepped back by the 16 ids.
    if(id>=16)
      addrhi += DWC_EMAC_MACHI16-DWC_EMAC_MACHI0-(16U<<3);
    alt_write_word( addrhi,   hi );
    alt_write_word( addrhi+4, lo );
  }
}



/// 
static bool emac_mac_init(const uint8_t* selfMac) {
  bool ok = true;
  uint16_t phyr = emac_phy_read(0);
  fprintf(stderr,"PHY[0]:%04x\r\n", phyr );
  fprintf(stderr,">MACCtrl/Sts:0x%08x\r\n", (unsigned)alt_read_word(DWC_EMAC_CTLSTS) );
  // TODO: We *wait* here for the link to be up but such behaviour should be
  // at least optional if not removed.
  emac_int_decode(); //TODO debug
  tPartnerSts partnerSts = 0;
  bool firstLinkCheck = true;
  bool linkUp = false;
  while(!linkUp) {
    partnerSts = emac_phy_get_partner_sts();
    linkUp =( PARTNER_LINKUP & partnerSts );
    for(int i=0;(linkUp||firstLinkCheck)&&(i<11);++i) {
      phyr = emac_phy_read(i);
      fprintf(stderr,"PHY[%x]:%04x\r\n", i, phyr );
    }
    if(linkUp||firstLinkCheck) {
      fprintf(stderr,"Link-Up:%c\r\n", linkUp?'y':'n' );
      fprintf(stderr,">MACCtrl/Sts:0x%08x\r\n", (unsigned)alt_read_word(DWC_EMAC_CTLSTS) );
      emac_int_decode(); //TODO debug
    }
// TODO: Is the (RO) DWC_EMAC_CTLSTS telling us what we should set our DWC_EMAC_MACCONFIG below to??? 
// It seems to tell us link-up(bit3) and duplex(bit0) but report our link-speed setting not tell us what it should be (maybe the same is true of duplex).
    firstLinkCheck = false;
  }
  // NOTE: We could use the CTLSTS reg tell us all of: link-up; speed [10->2.5|100->25|1000->125]; duplex.
  // HOWEVER, there is some hysteresis between link-up and clock speed settled.
  // So we would have to do: link-up; delay n; read CTLSTS and use it to set MAC Config.
  for(int i=0;i<5;++i)
    fprintf(stderr,">MACCtrl/Sts:0x%08x\r\n", (unsigned)alt_read_word(DWC_EMAC_CTLSTS) );
  // ----
  // Set own MAC addr. NOTE: littleendian and hi,lo order means bytes read in reverse.
  emac_set_mac0( selfMac );
  //
  // [5] Filter:
  //     0 = accept bdcst; perfect da match i.e. unicast to us.
  emac_set_filter( 0 ); //EMAC_FILTER_RX_ALL  |  EMAC_FILTER_PROMISCUOUS  );
  //
  // [6] TODO: set flow control @ DWC_EMAC_BASE+0x0018
  //
  // [7] TODO: set interrupt mask
  alt_write_word( DWC_EMAC_INTMSK, 0x0600U );
  //
  // [8] Set MAC config basic then add Tx&Rx enable
  emac_mac_config();


  return ok;
}

/// 
bool emac_core_init(t_rxCallBack rx, const uint8_t* selfMac, t_phyInit phyInit) {
  bool ok = rx && selfMac;
  ok &= emac_dma_init();
  emac_phy_reset();
  // A specialised PHY initialisation routine is optional.
  if(ok && phyInit)
    ok = phyInit();

  //emac_phy_loopback1000(); // TODO: TEST<<<<

  ok = ok && emac_mac_init( selfMac );
  rxCallBack = rx;
  return ok;
}

/// TODO:Needed other than for debug?
void emac_int_decode() {
  const bool en = (ALT_E_TRUE == alt_int_dist_is_enabled( EMAC_INTERRUPT ) );
  const bool act = (ALT_E_TRUE == alt_int_dist_is_active( EMAC_INTERRUPT ) );
  const bool pend = (ALT_E_TRUE == alt_int_dist_is_pending( EMAC_INTERRUPT ) );
  const uint32_t mac_sts = alt_read_word( DWC_EMAC_INTSTS );
  const uint32_t dma_sts = alt_read_word( DWC_DMA_STATUS );
  fprintf(stderr,"Sts(epa=%d%d%d):MAC=0x%x;DMA=0x%x\r\n",en?1:0,pend?1:0,act?1:0,(unsigned)mac_sts,(unsigned)dma_sts); //DEBUG
}

/// Handle interrupt
void emac_int_handle() {
  const uint32_t dma_sts = alt_read_word( DWC_DMA_STATUS );
  alt_write_word( DWC_DMA_STATUS, 0x0001ffffU & dma_sts );
  const uint32_t mac_sts = alt_read_word( DWC_EMAC_INTSTS );
  // MAC ints cleared by reading *MII Ctl Sts Reg...
  if(mac_sts) {
    //fprintf(stderr,"MACCtrl/Sts:0x%08x\r\n", (unsigned)alt_read_word(DWC_EMAC_CTLSTS) );
    volatile uint32_t dummy_read = alt_read_word(DWC_EMAC_CTLSTS);
    (void)dummy_read;
    //
    emac_mac_config();
  }
}


