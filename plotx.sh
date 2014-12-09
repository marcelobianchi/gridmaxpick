#!/bin/bash 

##makecpt -Cgray -T0/1/0.002 > la.cpt
makecpt -Cgray -T-2/2/0.2 > la.cpt

grdimage $1 -JX26/18 -B0.5/50a100 -Cla.cpt -Xc -Yc > la.ps

gv la.ps

