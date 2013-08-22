#!/bin/sh

echo "compiling Cstress"		
g++ -ggdb3 -Wall -pthread src/Smain.cpp src/Smanual.cpp src/Srealist.cpp src/Siterative.cpp -Iext/tclap-1.2.1/include -o main 

