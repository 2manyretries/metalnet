
#ifndef _H_EMAC_APP
#define _H_EMAC_APP

#include <stdbool.h>
#include <inttypes.h>

#include "eip.h"

bool eap_init(t_phyInit phyInit);

tUdpId* eap_getPDstUdpId();

#endif

