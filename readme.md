
MetalNet
====
Bare metal Ethernet for the Altera SoC HPS.


Author: GIves (2manyretries)


About
----
MetalNet provides a bare-metal (no-OS) Ethernet capability for the Altera SoC HPS.

In general, we provision a SW driver for the Ethernet MAC hard IP and a limited
IP stack.

There is no current intent to support TCP/IP as this has a more nuanced behaviour and programs such as LWIP give this.
However it may be that the SW drivers provisioned here make for an easier port of a TCP/IP program to the Altera SoC HPS.

Refer to program/readme.md for full details of the MetalNet Capabilities.


Prerequisites
----
As a Developer/Builder/Reuser you will need to know basic networking (and relevant Linux commands).

You will need to:
- Provision MAC address for SoC (they should nominally be globally unique and so I can not tell you one), Perhaps you want to look at the MAC address used by your Linux SW running on the SoC target and reuse that? For more information on MAC addresses Denx offer a page on them: http://www.denx.de/wiki/view/DULG/WhereCanIGetAValidMACAddress
- Know the MAC address of your Development PC (well in principle we can construct MetalNet programs that deduce this dynamically from ARP and/or Ping etc., but best/useful to know it anyway).
- Make up IP addresses for SoC and Development PC on the same subnet (apply your settings on the latter).

See also program/readme.md.


Static ARP Entry
----
To prevent ARPs from a Linux host use:
> sudo /sbin/arp -s 192.168.241.21 00:17:b0:57:0f:1b
Where the IP and MAC addresses are those of the target board.

MetalNet can support ARP responses but you may not want the extraneous traffic in a real-time application.

Firewall
----
The PC firewall may need to be disabled or a hole punched to accept UDP Rx from the target board.



