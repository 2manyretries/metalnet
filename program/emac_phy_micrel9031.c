
#include <stdio.h>

#include "emac_phy_micrel9031.h"

// This is the module we are called-out from, but we inevitably use it to
// talk to the PHY.
#include "emac_core.h"

/*
  The MICREL 9031 PHY allows RGMII skew settings to be applied via SW to
  fix-up PCB tracking disparities.
  If your target board has this PHY it may well be using this facility.

  You can find out by looking at the Device Tree for a Linux Distro that
  works on your target board.

  The below is for the Terasic Atlas/DE0-Nano-SoC board.
  On the rhs we have the divisor-values (as specified in the PHY manual) that
  convert ps to PHY units and thereby give the values to write to the PHY.

  Defined in:    devTree41.dts
                        txd0-skew-ps = <0x0>;    //    0/60 =  0
                        txd1-skew-ps = <0x0>;    //    0/60 =  0
                        txd2-skew-ps = <0x0>;    //    0/60 =  0
                        txd3-skew-ps = <0x0>;    //    0/60 =  0
                        rxd0-skew-ps = <0x1a4>;  //  420/60 =  7
                        rxd1-skew-ps = <0x1a4>;  //  420/60 =  7
                        rxd2-skew-ps = <0x1a4>;  //  420/60 =  7
                        rxd3-skew-ps = <0x1a4>;  //  420/60 =  7
                        txen-skew-ps = <0x0>;    //    0/60 =  0
                        txc-skew-ps = <0x744>;   // 1860/60 = 31
                        rxdv-skew-ps = <0x1a4>;  //  420/60 =  7
                        rxc-skew-ps = <0x690>;   // 1680/60 = 28
*/

/// Apply the Atlas board-specific clock-skew settings in the PHY.
bool emac_phy_init_micrel9031_atlas() {
  // The MMD registers and the values to write to them.
  const uint16_t mmdreg[4] = {      4,      5,      6,      8 };
  const uint16_t mmdval[4] = { 0x0070, 0x7777, 0x0000, 0x3c00U|(31U<<5)|28U };
  // (Note the reg 8 MSbits are documented as 0 but read back from PHY non-0.)
  for(int i=0;i<4;++i)
    fprintf(stderr,"MMD[2,%u]=%08x\r\n", (unsigned)mmdreg[i], (unsigned)emac_phy_mmd_read(2, mmdreg[i]) );
  for(int i=0;i<4;++i)
    emac_phy_mmd_write( 2, mmdreg[i], mmdval[i] );
  for(int i=0;i<4;++i)
    fprintf(stderr,"MMD[2,%u]=%08x\r\n", (unsigned)mmdreg[i], (unsigned)emac_phy_mmd_read(2, mmdreg[i]) );
  return true;
}

