
MetalNet
====

Altera SoC HPS target program.

GIves


Capabilities
----
The MetalNet supports the following capabilities:
- IPv4 (only!).
- ARP: We can respond to ARPs; this should allow us to be placed in the ARP table of the host PC.
- Ping: We can respond to Ping; together with the ARP capability this means we should be able to Ping the target dynamically i.e. without needing a tstaic ARP table entry.
- UDP: We can send and receive UDP packets

Limitations/Known Problems/TODO
---
- The program is instrumented at present and demonstrates capabilities (ARP,Ping,UDP) but *not* performance. TODO
- TCP is not supported and there is no intent to do so.
- IPv6 is not currently supported but support is possible...
- No Jumbo packets - but the HW supports it so...


HW Prerequisites
----
- Altera SoC Development Target.
  - Supported targets are: Atlas/DE0-NanoSoC.
- Development PC (I am using Linux and suspect we will have trouble if you are not).
- Network cable connection between development PC and SoC target.
- USB-Serial cable connection between development PC and SoC target.
- USB-Blaster(II) connection between development PC and SoC target.

SW Prerequisites
----
- Altera SoC EDS (I am using 15.1)
- (Optional) Altera University Program (Monitor Program)

Configuration Prerequisites
----
In general you will want to configure the SoC target and host PC Ethernet compatibly.
This means giving them IP addresses on the same subnet.

For the SoC Target 
- MAC address for SoC
- IP address for SoC
- PHY skew data (if not one of the supported targets identified above).

You need to make your *own* "eth_config.h" file based on the above settings.
An example is provisioned (i.e. "eth_config.example.h") - just copy it and fill in the definitions with your values.

Build
----
- Start a SoC EDS development shell and navigate to this folder.
- Type: make

Execute
----
- You should be able to debug the program using:
  - DS-5 - supplied as part of SoC EDS (you will need a license for bare-metal target debug capability).
  - Alter University Program, Monitor Program
- Connect to the target, download and run the built program.

Installation
----
TODO


Tips
----

Static ARP Entry
----
To prevent ARPs from a Linux host use:
> sudo /sbin/arp -s  <IP>  <MAC>


Firewall
----
The PC firewall may need to be disabled or a hole punched to accept UDP rx.



