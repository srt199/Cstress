Cstress – A stress testing tool for servers using TCP connections.
------------------------------------------------------------------
------------------------------------------------------------------

Written by: Sergi Álvarez Triviño (srt1199@gmail.com)

Cstress is a tool that allows to send data through n connections simultaneously in order to test how the destination server reacts.


##·Modes:
1. Manual mode (-m): Once the n connections are set, the specified quantity of data is sent through every connection at once.

2. Iterative mode (-i): Once the n connections are set, k chunks of m bytes are sent through all of them simultaneously, (one chunk after another).

3. Realistic mode (-r): It subjects the host to a simulation of n concurrent client/server communications during the specified period of time. (includes random connections/disconnections and sending of data in an unsynchronized way between clients.

Note: All the modes record the number of times a connection error happened and a server availability test before the end of the program execution.


##·Customizable Parameters: [mode]

-c: "connections": Number of concurrent connections to be made.

-a: "answer": Expected answer from the server.

-b: ”bytes":  Number of bytes sent on every connection[-m] or every iteration[-i][-r]

-k: "k_chunks": Number of chunks of bytes (iterations) sent on every connection[-m][-i]

-t: "time": Duration in seconds of the simulation[-r]

-d: "delimiter": delimiter (marks when the client has finished sending data)

-z: "ifile": File-path (data to be sent, e.g. a jpg image.)

-f: “force_simultaneity": With >1000 connections, forces to send bytes through all of them truly simultaneously (thus avoiding delays between them) (It can cause SIGSEGV depending on the configuration of your OS.


##·Usage Examples:

1. ``` ./main localhost 5008 -i -c 700 -b 5000 -k 15 -a "received" -d "**---//---**" ```

Summary: Connection to a server set in localhost, listening to port 5008. Mode: iterative. 700 concurrent connections. 15 chunks of 5000 bytes to be sent. Answer expected from server: received. Delimiter: **---//---**

2. ``` ./main localhost 5008 -m -c 1000 -b 7000 -a "received" ```

Summary: Connection to a server set in localhost, listening to port 5008. Mode: manual. 1000 concurrent connections. Only one chunk of 7000 bytes to be sent through every connection. Answer expected from server: received. (Without delimiters)

3. ``` ./main localhost 5008 -r -c 1000 -b 5000 -a "received" -d "**---//---**" -t 300 ```

Summary: Connection to a server set in localhost, listening to port 5008. Mode: realistic. 1000 concurrent connections. It sends chunks of 5000 bytes through every connection during a period of 300 seconds. (In an unsynchronized manner to do it a bit more realistic) Answer expected from server: received. Delimiter: **---//---**


##·Things that may be improved:

I know that a lot of things may be improved (this is basically a project born out of a need to do specifically this kind of testing to a server, as soon as possible.), but some of them are:

1. Code clarity: For instance separating some common functions in subroutines (e.g. host connection)

2. Time measurement performance: Improve the time measurement of the clients/server communication.

3. Reliability when spawning more than 1000 connections (threads): It may be a system-dependent issue, but the reliability of the program certainly decreases after breaking the 1000 connections barrier.




##·Build instructions:

####-Possible requisites:

``` ulimit -s 3500	 ```// decrease stack size

``` ulimit -n 13000	``` // increase num of open files allowed

// increase max connections allowed:
``` echo 5000 > /proc/sys/net/core/somaxconn ```

// change net.ipv4.ip_local_port_range :
``` sudo echo 15000 61000 > /proc/sys/net/ipv4/ip_local_port_range ```


####-Build:

Execute the file “build.sh”.




##·License 

   Cstress Copyright 2013 Sergi Álvarez Triviño

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.




