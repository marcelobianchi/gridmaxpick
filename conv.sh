#/bin/bash

G=$1

## Initialize
outf=_D
tmp=$(mktemp)
cp -v "$G" "$tmp"|| exit

## 
# Here you set the limits for plotting and resolution
#
#grdsample -Gn ${tmp} -I0.02/0.2 -R-2.0/0.3/-200/200; mv n ${tmp}

echo ""
echo "Some info for the mind"
echo ""
grdinfo -C ${tmp}

# HEADER
echo ""
echo "HEADER -> ${outf}"
grdinfo -C ${tmp} | awk '{print $2,$3,$8,$10"\n"$4,$5,$9,$11}' > ${outf}
echo "DATA -> ${outf}"
grd2xyz -ZBLa ${tmp} >> ${outf}
echo "DONE, file is ${outf}"

# CLEAN UP
rm -f ${tmp} .gmtcommands4
