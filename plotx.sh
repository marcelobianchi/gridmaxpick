#!/bin/bash 

G="$1"
PK="$2"

##makecpt -Cgray -T0/1/0.002 > la.cpt
makecpt -Cgray -T-2/2/0.2 > la.cpt

R=$(grdinfo -C  ${G} | awk '{printf "-R%.2f/%.2f/%.2f/%.2f",$2,$3,$4,$5}')
grdimage $R $G -JX26/18 -Cla.cpt -Xc -Yc -K -B1/100 > la.ps
[ -f "$PK" ] && psxy $PK -R -J -O -K -m -O -K >> la.ps
psbasemap -R -J -O -B0 >> la.ps

gv la.ps
rm -f la.cpt .gmtcommands4