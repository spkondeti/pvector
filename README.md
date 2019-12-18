# Path Vector Protocal Simulation (C Language, Socket Programming)
This is a Simulation of the Path Vector Protocol for Routers
This simulaton supports killing and reviving any router at any point of time
### Tech
This project uses the following:
* C language
* Socket Programming
* Threads
* UDP sockets
* Network Emulator (included in the repo as ne)
### Arguments for router and network emulator
• router <router id> <ne hostname> <ne UDP port> <router UDP port>
• ne <ne UDP Port number> <ConfigFile> 

### Instructions
* ConfigFiles are included inside examples directory of this repo
* Router ID's start from 0 (0 - 9 for 10 routers)
* The simulation starts only after firing up all the routers specified in the ConfigFile
* router<num>.log files will be created in the current directory (n log files for n routers)
* Every router's log file will be updated when there is an update/event

### Development
Want to contribute? Great!
Send commit requests or email me directly at skondeti@purdue.edu or suryapkondeti@gmail.com
