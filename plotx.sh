#!/bin/bash 

makecpt -Cgray -T0/1/0.002 > la.cpt
grdimage x.grd -JX26/18 -B0.5/50a100 -Cla.cpt -Xc -Yc > la.ps

gv la.ps

