
#ifndef _EMAC_CORE_H
#define _EMAC_CORE_H

#include <stdbool.h>

#include <alt_int_device.h>
// Level-triggered interrupt.
#define EMAC_INTERRUPT      ALT_INT_INTERRUPT_EMAC1_IRQ

typedef bool (*t_rxCallBack)(uint8_t* pbuf, uint16_t size);

#include "ephy.h" //TODO
/// Must be called first.
/// - rxCallBack: Mandatory routine to be called when we receive a packet.
/// - selfMac: Mandatory; our own MAC address.
/// - phyInit: Optional specialised routine to call to initialise PHY.
bool emac_core_init(t_rxCallBack rxCallBack, const uint8_t* selfMac, t_phyInit phyInit);

void emac_int_decode();

void emac_int_handle();

bool emac_is_link_up();

// ----
/// Write a MMD register in the PHY.
void emac_phy_mmd_write( uint8_t mmddev, uint16_t mmdreg, uint16_t mmdval );
/// Read a MMD register from the PHY.
uint16_t emac_phy_mmd_read( uint8_t mmddev, uint16_t mmdreg );
// ----

// There are 'perfect' MAC filters id 1..127.
// Each defaults off.
// For each we can turn on, filter src (instead of dst) address,
// ignore (don't care) about MAC byte 0..5.
typedef uint8_t tMacFilterMask;
#define MACFILTER_ON              (1U<<7)
#define MACFILTER_IS_SRC          (1U<<6)
#define MACFILTER_IGNORE_BYTE(N)  ((1U<<N)&0x03fU)
// Note MAC0 is reserved for self address, mandatory and special so not allowed
// here.
void emac_set_mac_filter( uint8_t id, tMacFilterMask mask, const uint8_t* pmac );



/// Call this regularly to process (via the rxCallBack installed at init)
/// anything received since last time.
void emac_rx_service();

/// Attempt to allocate a Tx descriptor returning a handle to it and
/// its associated buffer into which we put our packet.
/// Note we may fail to have a Tx buffer available (e.g. if transmitting
/// faster than the link) and return 0.
uint32_t* emac_tx_alloc(uint8_t** ppbuf);
/// Complete a previous allocation by requesting transmission.
void emac_tx_complete(uint32_t *pdesc, uint32_t buf_used);
///
void emac_tx_complete_with_bdy(uint32_t *pdesc, uint32_t hdr_used, uint8_t* pbdy, uint32_t bdy_used);

#endif

