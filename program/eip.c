#include <stdio.h>

/// The custom configuration of our platform.
#include "eth_config.h"

// TODO: Add PT to get SCTLR working...(?)
#include <string.h>

#include "eip.h"

#include "emac_core.h"

#define ETHERTYPE_ETHERNET     0x0800U
#define ETHERTYPE_ARP          0x0806U

//----
// VLAN
// VLAN tagging adds 4 bytes between the SrcMAC and (encapsulated) Ethertype.
// The 4 bytes comprise of an Ethertype indicating 'VLAN' followed by 2 bytes
// conveying the VID (last 12bits), QoS (first 4 bits).
// - A VID of 0 means only the QoS info is relevant (not VLANed).
// - A VID of FFF is reserved and means ???.
//
// 802.1q : ref. Wikipedia - the 'normal' frame continues at +4:
#define ETHERTYPE_VLAN         0x8100U
//
// 802.1ad: ref. Wikipedia - double-tag, so expect an 802.1q to follow and then 'normal' frame i.e. at +8:
#define ETHERTYPE_VLAN2A       0x88a8U
// Non-standard variant of double-tagging.
#define ETHERTYPE_VLAN2B       0x9100U
// Apparently more than 2 VLAN tags can be prepended and tag values are ??? as above ???
//----



#define ETHERTYPE_HI(X)  (X>>8)
#define ETHERTYPE_LO(X)  (X&0xffU)


static const tMacId selfMac = {
  ETHCFG_SELF_MAC0,
  ETHCFG_SELF_MAC1,
  ETHCFG_SELF_MAC2,
  ETHCFG_SELF_MAC3,
  ETHCFG_SELF_MAC4,
  ETHCFG_SELF_MAC5
};

static const tIpId selfIp = {
  ETHCFG_SELF_IP4_0,
  ETHCFG_SELF_IP4_1,
  ETHCFG_SELF_IP4_2,
  ETHCFG_SELF_IP4_3
};


#define IP_IS_ICMP  0x01U
#define IP_IS_UDP   0x11U

#define CPY_MAC(DST,SRC)  for(int i=0;i<MAC_SIZE;++i) ((uint8_t*)(DST))[i] = ((uint8_t*)(SRC))[i]
#define CPY_IP4(DST,SRC)  for(int i=0;i<IP4_SIZE;++i) ((uint8_t*)(DST))[i] = ((uint8_t*)(SRC))[i]


uint16_t getVlanSize(uint8_t *at, uint16_t len) {
  uint16_t vlansize = 0;
  //TODO: while data at ethertype+vlansize offset indicates vlan,
  //      and len-vlansize is that of a valid packet,
  //      add 4 to vlansize.
  return vlansize;
}



// Returns location to place body.
uint8_t *mkEthIpv4UdpHeader(uint8_t *at, uint16_t bdyLen, tUdpId* dst) {

  //*at = 0xd5U; //SFD;
  //++at;

  // 0.. 5 = dmac //TODO<<<param
  at[0] = dst->pdmacip->mac[0];
  at[1] = dst->pdmacip->mac[1];
  at[2] = dst->pdmacip->mac[2];
  at[3] = dst->pdmacip->mac[3];
  at[4] = dst->pdmacip->mac[4];
  at[5] = dst->pdmacip->mac[5];
  // 6..11 = smac/sha
//memcpy( &at[6], selfMac, MAC_SIZE );
  at[6] = ETHCFG_SELF_MAC0;
  at[7] = ETHCFG_SELF_MAC1;
  at[8] = ETHCFG_SELF_MAC2;
  at[9] = ETHCFG_SELF_MAC3;
  at[10]= ETHCFG_SELF_MAC4;
  at[11]= ETHCFG_SELF_MAC5;
  //12..13 = etherType/size
  at[12]= ETHERTYPE_HI(ETHERTYPE_ETHERNET);
  at[13]= ETHERTYPE_LO(ETHERTYPE_ETHERNET);
  return mkIpv4UdpHeader( &at[14], bdyLen, dst );
}

// Returns location to place body.
// Note: pkt size must be min 64 - need to pad if bdyLen small.
uint8_t *mkIpv4UdpHeader(uint8_t *at, uint16_t bdyLen, tUdpId* dst ) {
  static uint16_t ident = 0;
  ++ident;
  const uint16_t sport = dst->sport; //TODO<<<param
  const uint16_t dport = dst->dport; //TODO<<<param
  const uint16_t udpLen = 8+bdyLen;
  const uint16_t totLen = 20+udpLen;
  //
  at[0] = 0x45U; // IPv4, 5xword32 header len
  at[1] = 0x00U; // DSCP, ECN
  at[2] = totLen>>8;
  at[3] = totLen;
  at[4] = ident>>8;
  at[5] = ident;
  at[6] = 0x40U; // Don't frag, not a frag
  at[7] = 0x00U;
  at[8] = 0x40; //TTL
  at[9] = IP_IS_UDP; //UDP
  // 10,11 = IPv4 checksum
  at[10] = 0;
  at[11] = 0;
  at[12]= ETHCFG_SELF_IP4_0; //src
  at[13]= ETHCFG_SELF_IP4_1;
  at[14]= ETHCFG_SELF_IP4_2;
  at[15]= ETHCFG_SELF_IP4_3;
  at[16]= dst->pdmacip->ip[0];
  at[17]= dst->pdmacip->ip[1];
  at[18]= dst->pdmacip->ip[2];
  at[19]= dst->pdmacip->ip[3];
  // vvvv UDP part: ports, len, chk.
  at[20]= sport>>8;
  at[21]= sport;
  at[22]= dport>>8;
  at[23]= dport;
  at[24]= udpLen>>8;
  at[25]= udpLen;
  //26,27 = UDP checksum
  at[26]= 0;
  at[27]= 0;
  return &at[28];
}


static uint8_t *mkArpRBody(uint8_t *at, const uint8_t *fromArp, uint16_t vlansize) {
  at[0] = 0;
  at[1] = 1; // HTYPE = Ethernet (type of intended protocol)
  at[2] = ETHERTYPE_HI(ETHERTYPE_ETHERNET);
  at[3] = ETHERTYPE_LO(ETHERTYPE_ETHERNET);
  at[4] = 6; //HLEN
  at[5] = 4; //PLEN
  //
  at[6] = 0;
  at[7] = 2; // 1=request; 2=response
  // We are the src of the response...
  // 8..13 = smac/sha
//memcpy( &at[8], selfMac, MAC_SIZE );
  at[8] = ETHCFG_SELF_MAC0;
  at[9] = ETHCFG_SELF_MAC1;
  at[10]= ETHCFG_SELF_MAC2;
  at[11]= ETHCFG_SELF_MAC3;
  at[12]= ETHCFG_SELF_MAC4;
  at[13]= ETHCFG_SELF_MAC5;
  at[14]= ETHCFG_SELF_IP4_0; //src
  at[15]= ETHCFG_SELF_IP4_1;
  at[16]= ETHCFG_SELF_IP4_2;
  at[17]= ETHCFG_SELF_IP4_3;
  // Target MAC
  //memcpy( &at[18], &fromArp[6], MAC_SIZE );
  CPY_MAC( &at[18], &fromArp[6] );
  // Target IP
  //memcpy( &at[24], &fromArp[14+14], 4 );
  CPY_IP4( &at[24], &fromArp[14+14+vlansize] );
  // Runts not allowed!
  for(int i=28;i<50;++i)
    at[i] = 0;
  return &at[50];
}


// Make a response to an ARP.
uint8_t *mkEthArpR(uint8_t *at, const uint8_t *fromArp, uint16_t vlansize) {
printf("ArpR(0x%x,0x%x)\r\n",(unsigned)at,(unsigned)fromArp); //DEBUG
  // Our dst is the input src.
  //memcpy( &at[0], &fromArp[6], MAC_SIZE );
  //memcpy( &at[6], selfMac, MAC_SIZE );
  //for(int i=0;i<MAC_SIZE;++i) at[i] = 0xffU; // broadcast response?
  CPY_MAC( &at[0], &fromArp[6] ); // unicast response?
  CPY_MAC( &at[6], selfMac );
  // TODO: Replace this with a copy of the whole VLAN stuff: from 12..12+1+vlansize ????
  //12..13 = etherType/size
  at[12]= ETHERTYPE_HI(ETHERTYPE_ARP);
  at[13]= ETHERTYPE_LO(ETHERTYPE_ARP);
//printf("mkArpRBody\r\n");
  return mkArpRBody( &at[14], fromArp, vlansize );
}

bool isArpMe(const uint8_t *at, uint16_t vlansize) {
  // EtherType is ARP and a request
  printf("isarp?@%x\r\n",(unsigned)at); //DEBUG
  bool arp = 
    (ETHERTYPE_HI(ETHERTYPE_ARP) == at[12+vlansize]) &&
    (ETHERTYPE_LO(ETHERTYPE_ARP) == at[13+vlansize]) &&
    (0 == at[14+vlansize]) &&
    (1 == at[15+vlansize]);
  // Dst IP is me.
  if(arp) {
    for(int i=0;arp && (i<IP4_SIZE); ++i)
      arp = (at[14+24+i+vlansize] == selfIp[i]);
  }
  // Option (a): Dst MAC is broadcast
  bool arp_init=arp;
  for(int i=0;i<MAC_SIZE && arp_init; ++i)
    arp_init = (at[i] == 0xffU);
  // Option (b): Dst MAC is me
  bool arp_refresh=arp;
  for(int i=0;i<MAC_SIZE && arp_refresh; ++i)
    arp_refresh = (at[i] == selfMac[i]);
  // Dst MAC is *either* bdcst *or* me.
  arp &= (arp_init|arp_refresh);
  // Src MAC is not me
  if(arp) {
    int match=0;
    for(int i=0;i<MAC_SIZE; ++i)
      if(at[6+i] == selfMac[i])
        ++match;
    arp = (match != MAC_SIZE);
  }
  //  
  return arp;
}


// Packet is Ether; dst mac is me.
bool isEtherMe(const uint8_t *at, uint16_t vlansize) {
  bool ether = 
    (ETHERTYPE_HI(ETHERTYPE_ETHERNET) == at[12+vlansize]) &&
    (ETHERTYPE_LO(ETHERTYPE_ETHERNET) == at[13+vlansize]);
  for(int i=0;ether&&(i<MAC_SIZE);++i)
    ether =( at[i]== selfMac[i] );
  return ether;
}

// Packet is IPv4 (?TODO?: NO OPTIONS), dst ip is me.
bool isIpv4Me(const uint8_t *at) {
  bool ip = at[0] == 0x45U;
  for(int i=0;ip&&(i<IP4_SIZE);++i)
    ip =( at[16+i]== selfIp[i] );
  return ip;
}

// Note we want to be passed IP pkt.
bool isPingRequest(const uint8_t *at) {
  bool ping =
    IP_IS_ICMP==at[9] &&
    8U ==at[20] &&
    0U ==at[21];
  return ping;
}

// Returns non-zero body size if ok - TODO: What about legit 0 bodies?!
uint16_t isUdpMe(const uint8_t *at, uint16_t dport) {
  uint16_t bdyLen = 0;
  bool udp =
    IP_IS_UDP==at[9] &&
    at[16]== ETHCFG_SELF_IP4_0 && //dst
    at[17]== ETHCFG_SELF_IP4_1 &&
    at[18]== ETHCFG_SELF_IP4_2 &&
    at[19]== ETHCFG_SELF_IP4_3 &&
    at[22]==(dport>>8) &&
    at[23]==(dport&0xffU);
    //at[24]==(udpLen>>8) &&
    //at[25]==(udpLen&0xffU);
  if(udp) {
    const uint16_t udpLen = (at[24]<<8)| at[25];
    if((udpLen>=8U)&&(udpLen<1512U))
      bdyLen = udpLen-8U;
  }
  return bdyLen;
}

/// Make a Ping response from the provided input Ping message.
/// It is expected that the input Ping has already been confirmed to refer to
/// us and therefore we can rip our response data from it.
uint8_t *mkEthPingResponse(uint8_t *at, const uint8_t *fromPing, uint16_t vlansize) {
  // Construct Ether header swapping src&dst mac.
  CPY_MAC( &at[0], &fromPing[6] );
  CPY_MAC( &at[6], selfMac );
  // TODO: Copy the from 12..12+1+vlansize and shift up what follows?
  at[12]= ETHERTYPE_HI(ETHERTYPE_ETHERNET);
  at[13]= ETHERTYPE_LO(ETHERTYPE_ETHERNET);
  // Copy IP header swapping src&dst ip.
  uint8_t *ip=&at[14]; // TODO:vlansize not applied here but is on other side!?
  memcpy( ip, &fromPing[14+vlansize], 10);
  // IPv4 checksum - seems to work if not zeroed i.e. part of the memcpy() above.
  ip[10] = 0;
  ip[11] = 0;
  CPY_IP4( &ip[12], &fromPing[14+16+vlansize] );
  CPY_IP4( &ip[16], &fromPing[14+12+vlansize] );
  const int totLen =( (((uint16_t)ip[2])<<8) | ((uint16_t)ip[3]) );
  const int pingLen= totLen-20;
  //printf("[%d,%d]\r\n",totLen,pingLen);
  if(pingLen>=8) {
    ip[20] = 0U; //Ping response ('from' is a request).
    memcpy( &ip[21], &fromPing[14+21+vlansize], pingLen-1 );
  }
  // YES: the ICMP HW chksum is additive i.e. we need to zero the field itself.
  ip[22] = 0;
  ip[23] = 0;
  //
  return &ip[totLen];
}



// ----
/***
#include "emac_core.h"
#include "emac_sys.h"

#define MY_UDP_PORT  4950U

// TODO: This is really an *example* of how to behave.
// - We respond to ARP from anyone.
// - We respond to ping from anyone.
// - We accept UDP
bool eip_rxpkt_process(uint8_t *pPktIn, uint16_t pktSize8) {
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
***/

bool eip_init(t_rxCallBack rxCallBack, t_phyInit phyInit) {
  bool ok = emac_core_init( rxCallBack, selfMac, phyInit );
  return ok;
}

// --

bool eip_txpkt_udp(uint8_t *pUdpBdy, uint16_t udpBdySize8, tUdpId *pDstUdpId) {
  bool ok = pUdpBdy && pDstUdpId;
  uint32_t* ptxdesc=0;
  uint8_t*  ptxbuf =0;
  uint8_t*  pBdyDst=0;
  ptxdesc = emac_tx_alloc(&ptxbuf);
  ok = ptxdesc && ptxbuf;
  if(ok) {
    pBdyDst = mkEthIpv4UdpHeader( ptxbuf, udpBdySize8, pDstUdpId );
    ok = pBdyDst;
    if(ok)
      memcpy( pBdyDst, pUdpBdy, udpBdySize8);
  }
  if(ok) {
    uint32_t totsize = (pBdyDst-ptxbuf)+udpBdySize8;
    if(totsize < 64) { //TODO:max size?
      totsize=64;
      //TODO: zero out the residue to prevent data leakage?
    }
    emac_tx_complete( ptxdesc, totsize);
  }
  return ok;
}

// Send UDP pkt without copying the body.
// NOTE/TODO: Beware of body alignment/size - ideally both are cache-aligned,
// but moreover non-u32 aligned or non-cache-aligned may not work???
// Note body size alignment matters only in terms of ensuring alignment of next
// data in a contiguous block send.
// NOTE/TODO: How does caller know when their body has been consumed and space
// can be reclaimed?
// - If we know the number of tx queue entries is n then after emac accepts n
//   more, we know that a given entry can be released.
//   So caller should maintain >=n+1 body slots.
// - We can do something with call-backs from the tx interrupt but then must
//   maintain n pointers to callbacks and have an interrupt on each tx.
// - If emac maintains and provides output on number of tx delivered minus
//   allocated, or equivalently current queue length (number pending),
//   then: caller knows (assuming only one caller) number of bodies pending.
bool eip_txpkt_udp_zc(uint8_t *pUdpBdy, uint16_t udpBdySize8, tUdpId *pDstUdpId) {
  bool ok = pUdpBdy;
  uint32_t* ptxdesc=0;
  uint8_t*  ptxbuf =0;
  uint8_t*  pBdyDst=0;
  ptxdesc = emac_tx_alloc(&ptxbuf);
  ok = ptxdesc && ptxbuf;
  if(ok) {
    pBdyDst = mkEthIpv4UdpHeader( ptxbuf, udpBdySize8, pDstUdpId );
    ok = pBdyDst;
  }
  if(ok) {
    uint32_t hdrsize = (pBdyDst-ptxbuf);
    // Reject runts since we do not control the body to pad them.
    ok =(hdrsize+udpBdySize8 >= 64);
    if(ok) {
      emac_tx_complete_with_bdy( ptxdesc, hdrsize, pUdpBdy, udpBdySize8);
    }
  }
  return ok;
}

