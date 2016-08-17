
#ifndef _EPHY_H
#define _EPHY_H

#define PHY_R0_RESET_BIT           15
#define PHY_R0_LOOPBACK_BIT        14
#define PHY_R0_SPEED_100_BIT       13
#define PHY_R0_AUTONEG_BIT         12
#define PHY_R0_POWER_DOWN_BIT      11
#define PHY_R0_ISOLATE_BIT         10
#define PHY_R0_RESTART_AUTONEG_BIT  9
#define PHY_R0_FULL_DUPLEX_BIT      8
#define PHY_R0_SPEED_1000_BIT       6

// SPEED:bits(6,13): (1,0)=1000Mbps; (0,1)=100Mbps; (0,0)=10Mbps
#define PHY_R0_SPEED_1000          (PHY_MASK(0,SPEED_1000))
#define PHY_R0_SPEED_100           (PHY_MASK(0,SPEED_100))
#define PHY_R0_SPEED_10            (0)


#define PHY_R1_100BASET4_BIT       15
#define PHY_R1_100BASETX_FULL_BIT  14
#define PHY_R1_100BASETX_HALF_BIT  13
#define PHY_R1_10BASET_FULL_BIT    12
#define PHY_R1_10BASET_HALF_BIT    11
#define PHY_R1_EXT_STATUS_BIT       8
#define PHY_R1_NO_PREAMBLE_BIT      6
#define PHY_R1_AUTONEG_DONE_BIT     5
#define PHY_R1_REMOTE_FAULT_BIT     4
#define PHY_R1_AUTONEG_CAP_BIT      3
#define PHY_R1_LINK_UP_BIT          2
#define PHY_R1_JABBER_BIT           1
#define PHY_R1_EXT_CAP_BIT          0

// R5 status to work out auto-neg result
// (T4 is inherently half-duplex according to Wikipedia)
#define PHY_R5_100BASET4_HALF_BIT   9
#define PHY_R5_100BASETX_FULL_BIT   8
#define PHY_R5_100BASETX_HALF_BIT   7
#define PHY_R5_10BASET_FULL_BIT     6
#define PHY_R5_10BASET_HALF_BIT     5

// R10 (0xa) status to work out gige auto-neg result
#define PHY_R10_1000BASET_FULL_BIT 11
#define PHY_R10_1000BASET_HALF_BIT 10




// Shorthand: for Reg N, bit-weight I
#define PHY_MASK(N,I) (1U<<PHY_R##N##_##I##_BIT)


#include <stdbool.h>
typedef bool (*t_phyInit)();


#endif

