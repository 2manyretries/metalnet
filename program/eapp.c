
#include <stdio.h>

#include "eapp.h"

#include "emac_core.h"
#include "emac_sys.h"

#include "eip.h"


/// The custom configuration of our partner platform.
#include "eth_config.h"


/// UDP port matches host programs.
#define MY_UDP_PORT  4950U

// This is an *example* of how to behave.
// - We respond to ARP from anyone.
// - We respond to ping from anyone.
// - We accept UDP
static bool eap_rxpkt_process(uint8_t *pPktIn, uint16_t pktSize8) {
  bool ok = true;
  uint32_t* ptxdesc=0;
  uint8_t*  ptxbuf =0;
  uint8_t*  ptxbend=0;

  const uint16_t vlandatasize = getVlanSize(pPktIn,pktSize8);
  if(isArpMe(pPktIn,vlandatasize)) { // ARP at buff addr?
    //printf( "ARP!\r\n" );
    ptxdesc = emac_tx_alloc(&ptxbuf);
    ok = ptxdesc && ptxbuf;
    if(ok) {
      ptxbend = mkEthArpR(ptxbuf, pPktIn, vlandatasize );
      emac_tx_complete( ptxdesc, (uint32_t)(ptxbend-ptxbuf));
    }
  } else if (isEtherMe(pPktIn,vlandatasize)) {
    printf( "EtherMe\r\n" );
    if (isIpv4Me(&pPktIn[14+vlandatasize])) {
      printf( "IPv4Me\r\n" );
      if (isPingRequest(&pPktIn[14+vlandatasize])) {
        printf( "PING!\r\n" );
        ptxdesc = emac_tx_alloc(&ptxbuf);
        ok = ptxdesc && ptxbuf;
        if(ok) {
          ptxbend = mkEthPingResponse(ptxbuf, pPktIn, vlandatasize);
          emac_tx_complete( ptxdesc, (uint32_t)(ptxbend-ptxbuf));
        }
      } else {
        uint16_t len = isUdpMe(&pPktIn[14+vlandatasize], MY_UDP_PORT);
        if(len)
          fprintf(stderr,"UDP port %u len %u\r\n",MY_UDP_PORT,(unsigned)len);
      }
    }
  }
  if(!ok) printf("ERR:rxpkt_process\r\n"); //DEBUG
  return ok;
}

// TODO: This needs to be pulled out... Fixed link partner details...
static tMacIpId dstMacIpId = {
  .mac = {
    ETHCFG_PARTNER_MAC0,
    ETHCFG_PARTNER_MAC1,
    ETHCFG_PARTNER_MAC2,
    ETHCFG_PARTNER_MAC3,
    ETHCFG_PARTNER_MAC4,
    ETHCFG_PARTNER_MAC5
  },
  .ip  = {
    ETHCFG_PARTNER_IP4_0,
    ETHCFG_PARTNER_IP4_1,
    ETHCFG_PARTNER_IP4_2,
    ETHCFG_PARTNER_IP4_3
  }
};

static tUdpId dstUdpId = {
  .pdmacip = &dstMacIpId,
  .sport   = MY_UDP_PORT,
  .dport   = MY_UDP_PORT
};
// --

bool eap_init(t_phyInit phyInit) {
  // First fix-up/check the HPS system around the EMAC first.
  bool ok = emac_sys_init();
  // Now initialise the EMAC itself.
  ok &= eip_init( eap_rxpkt_process, phyInit );
  return ok;
}


tUdpId* eap_getPDstUdpId() {
  return &dstUdpId;
}

