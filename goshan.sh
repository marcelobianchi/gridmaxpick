#!/bin/bash 

# Compile
gcc -c -m64 -pipe -O2 -Wall -W -DQT_WEBKIT -I/usr/share/qt4/mkspecs/linux-g++-64 -I. -I/opt/local/include -o main.o main.c
gcc -c -m64 -pipe -O2 -Wall -W -DQT_WEBKIT -I/usr/share/qt4/mkspecs/linux-g++-64 -I. -I/opt/local/include -o interaction.o interaction.c
g++ -m64 -o testegrid main.o interaction.o  -lcpgplot  -L/opt/local/lib/ -lm

# VARIABLES
export LD_LIBRARY_PATH=/opt/local/lib:$LD_LIBRARY_PATH
export PGPLOT_DIR=/opt/local/share/pgplot/
export PATH=/opt/local/bin:$PATH

# RUN
./testegrid
