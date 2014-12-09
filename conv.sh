#/bin/bash

G=$1

#Here you set the limits for plotting and resolution

#grdsample -Gn $G -I0.02/0.2 -R-2.0/0.3/-200/200
#cp n xn.gdr

cp -iv $G xn.grd
grdinfo -C xn.grd

#Here we export to ASCII
grdinfo -C xn.grd | awk '{print $2,$3,$8,$10"\n"$4,$5,$9,$11}' > _D
grd2xyz -ZBLa xn.grd >> _D 
