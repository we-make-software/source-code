# **ThePostOffice - Kernel Packet Handling Module**  

`ThePostOffice` is a Linux kernel module designed to capture incoming network packets and process them before passing them to `TheMailConditioner`. It registers a packet handler and filters out unwanted traffic before queueing packets for further processing.


This module is designed for efficiency, using work queues to handle packets asynchronously instead of blocking the network stack. It ensures minimal overhead while processing packets in a structured manner.

## License
This project is licensed under the **Do What The F*ck You Want To Public License (WTFPL)**.  
See the [LICENSE](LICENSE) file for details.
