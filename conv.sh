#/bin/bash

G=x.grd

#Here you set the limits for plotting and resolution
#grdsample -Gn $G -I0.05/2 -R-2.0/0.3/-200/200
cp x.grd xn.gdr

#Here we export to ASCII
grdinfo -C xn.gdr | awk '{print $2,$3,$8,$10"\n"$4,$5,$9,$11}' > _D
grd2xyz -Z xn.gdr >> _D 

