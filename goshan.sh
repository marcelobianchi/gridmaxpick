#!/bin/bash 

# If gmt.h file is in /usr/include/gmt/gmt.h add here /usr/include
#
GMT="/opt/local/lib/gmt4/include"
GMT="/usr/include/gmt"

# The folder where libgmt.so or in the mac, the dynamic file.
#
GMTLIB="/opt/local/lib/gmt4/lib"

# Checks
#
[ ! -f "$GMT/gmt.h" ] && echo "No file $GMT/gmt.h" && exit
[ ! -f "$GMTLIB/libgmt.dynlib" ] && echo "No file $GMTLIB/libgmt.dynlib" && exit

# Compile
#
gcc -c -m64 -O2 -W -DUSEGMTGRD -I. -I${GMT} -I/opt/local/include -o main.o main.c
gcc -c -m64 -O2 -W -DUSEGMTGRD -I. -I${GMT} -I/opt/local/include -o interaction.o interaction.c
gcc -o testegrid main.o interaction.o  -lcpgplot  -L/opt/local/lib/ -L${GMTLIB} -lm -lgmt

# VARIABLES
#
export LD_LIBRARY_PATH=/opt/local/lib:/opt/local/lib/gmt4/lib:$LD_LIBRARY_PATH
export PGPLOT_DIR=/opt/local/share/pgplot/
export PATH=/opt/local/bin:$PATH

# RUN
#
./testegrid
