#!/bin/bash 

# If gmt.h file is in /usr/include/gmt/gmt.h add here /usr/include
GMT="/opt/local/lib/gmt4/include"

# The folder where libgmt.so or in the mac, the dynamic file.
GMTLIB="/opt/local/lib/gmt4/lib"

[ -z "$GMT" ] && echo "EDIT ME " && exit

# Compile
gcc -c -m64 -pipe -O2 -Wall -W -DUSEGMTGRD -DQT_WEBKIT -I/usr/share/qt4/mkspecs/linux-g++-64 -I. -I$GMT -I/opt/local/include -o main.o main.c
gcc -c -m64 -pipe -O2 -Wall -W -DUSEGMTGRD -DQT_WEBKIT -I/usr/share/qt4/mkspecs/linux-g++-64 -I. -I$GMT -I/opt/local/include -o interaction.o interaction.c
g++ -m64 -o testegrid main.o interaction.o  -lcpgplot  -L/opt/local/lib/ -L$GMTLIB -lm -lgmt

# VARIABLES
export LD_LIBRARY_PATH=/opt/local/lib:$LD_LIBRARY_PATH
export PGPLOT_DIR=/opt/local/share/pgplot/
export PATH=/opt/local/bin:$PATH

# RUN
./testegrid
