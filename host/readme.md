MetalNet Host Tools
====

Here are programs to run on the host PC (Linux) to demonstrate interaction with
MetalNet running on an Ethernet connected target board.

The following programs are basically copies of free Beej network programs and the MetalNet example has been designed to compatible interaction.
Where appropriate the target interaction is explained below.

Refer to the Beej website and tutorials for more info on these examples.
https://beej.us/guide/bgnet/
(Note: The Beej document states the code 'is completely free of any license restriction.')


"talker" - UDP sender
----
Sends packet to target - specify the IP address and text to encapsulate in packet.

The target should report receipt of the packet.

"listener" - UDP Receiver
----
Listens for packet from target.
NOTE: You may need to disable or punch-a-hole-in your firewall for this.

A packet is sent from the target by typing a character into the target serial console.

