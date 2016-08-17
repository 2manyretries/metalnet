
MetalNet
====
Bare metal Ethernet for the Altera SoC HPS.


Greg Ives



Prerequisites
----
You need to provision:
- MAC address for SoC
- IP address for SoC
- PHY skew data


Static ARP Entry
----
To prevent ARPs from a Linux host use:
> sudo /sbin/arp -s 192.168.241.21 00:17:b0:57:0f:1b
Where the IP and MAC addresses are those of the target board.

Firewall
----
The PC firewall may need to be disabled or a hole punched to accept UDP Rx from the target board.



