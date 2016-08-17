
#ifndef _H_EIP
#define _H_EIP

#include <stdbool.h>
#include <inttypes.h>

// ----
// Internet Protocol (IPv4) over Ethernet ARP packet
// octet|            0                   |             1                  |
// -----+--------------------------------+--------------------------------|
// 0 	|                    Hardware type (HTYPE)                        |
// 2 	|                    Protocol type (PTYPE)                        |
// 4 	| Hardware address length (HLEN) | Protocol address length (PLEN) |
// 6 	|                    Operation (OPER)                             |
// 8 	|            Sender hardware address (SHA) (first 2 bytes)        |
// 10 	|                    (next 2 bytes)                               |
// 12 	|                    (last 2 bytes)                               |
// 14 	|            Sender protocol address (SPA) (first 2 bytes)        |
// 16 	|                    (last 2 bytes)                               |
// 18 	|            Target hardware address (THA) (first 2 bytes)        |
// 20 	|                    (next 2 bytes)                               |
// 22 	|                    (last 2 bytes)                               |
// 24 	|            Target protocol address (TPA) (first 2 bytes)        |
// 26 	|                    (last 2 bytes)                               |
// -----+--------------------------------+--------------------------------|

/****
#define IP_N0(B)  B<<

#define TO_ARP_HTYPE_PTYPE(H,P)         \
    ((H&0xffU)<<8U)|((H&0xff00U)>>8U)   \
    ((P&0xffU)<<24U)|((P&0xff00U)<<16U)
****/


#define MAC_SIZE 6
typedef uint8_t tMacId[MAC_SIZE];

#define IP4_SIZE 4
typedef uint8_t tIpId[IP4_SIZE];

typedef struct {
  tMacId mac;
  tIpId  ip;
} tMacIpId;

typedef uint16_t tPort;

typedef struct {
  tMacIpId* pdmacip;
  tPort     sport;
  tPort     dport;
} tUdpId;



// ----

uint16_t getVlanSize(uint8_t *at, uint16_t len);


uint8_t *mkEthIpv4UdpHeader(uint8_t *at, uint16_t bdyLen, tUdpId* dst );

uint8_t *mkIpv4UdpHeader(uint8_t *at, uint16_t bdyLen, tUdpId* dst );

uint8_t *mkEthGArp(uint8_t *at);

uint8_t *mkEthArpR(uint8_t *at, const uint8_t *fromArp, uint16_t vlangap);

bool isArpMe(const uint8_t *at, uint16_t vlangap);

bool isEtherMe(const uint8_t *at, uint16_t vlangap);

bool isIpv4Me(const uint8_t *at);

uint16_t isUdpMe(const uint8_t *at, uint16_t dport);

bool isPingRequest(const uint8_t *at);

uint8_t *mkEthPingResponse(uint8_t *at, const uint8_t *fromPing, uint16_t vlangap);

bool eip_rxpkt_process(uint8_t *pPktIn, uint16_t pktSize8);

#include "ephy.h" //TODO
#include "emac_core.h" //TODO
bool eip_init(t_rxCallBack rxCallBack, t_phyInit phyInit);

bool eip_txpkt_udp(uint8_t *pUdpBdy, uint16_t udpBdySize8, tUdpId *pDstUdpId);
// As above but without copying body.
bool eip_txpkt_udp_zc(uint8_t *pUdpBdy, uint16_t udpBdySize8, tUdpId *pDstUdpId);


#endif

