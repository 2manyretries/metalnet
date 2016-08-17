
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
> sudo /sbin/arp -s 192.168.241.221 00:1a:a0:59:0f:b3


Firewall
----
The PC firewall may need to be disabled or a hole punched to accept UDP rx.



