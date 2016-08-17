
#ifndef _H_ETH_CONFIG
#define _H_ETH_CONFIG


/// Our IPv4 address
#define ETHCFG_SELF_IP4_0    192U
#define ETHCFG_SELF_IP4_1    168U
#define ETHCFG_SELF_IP4_2    240U
#define ETHCFG_SELF_IP4_3    1U

/// Our MAC address
#define ETHCFG_SELF_MAC0   0x0U
#define ETHCFG_SELF_MAC1   0x1U
#define ETHCFG_SELF_MAC2   0x2U
#define ETHCFG_SELF_MAC3   0x3U
#define ETHCFG_SELF_MAC4   0x4U
#define ETHCFG_SELF_MAC5   0x5U



// For a fixed statically defined partner
#define ETHCFG_FIXED_PARTNER
#ifdef  ETHCFG_FIXED_PARTNER

/// Partner IPv4 address
#define ETHCFG_PARTNER_IP4_0    192U
#define ETHCFG_PARTNER_IP4_1    168U
#define ETHCFG_PARTNER_IP4_2    240U
#define ETHCFG_PARTNER_IP4_3    2U

/// Partner MAC address
#define ETHCFG_PARTNER_MAC0    0x0U
#define ETHCFG_PARTNER_MAC1    0x1U
#define ETHCFG_PARTNER_MAC2    0x2U
#define ETHCFG_PARTNER_MAC3    0x3U
#define ETHCFG_PARTNER_MAC4    0x4U
#define ETHCFG_PARTNER_MAC5    0x6U

#endif


#endif

