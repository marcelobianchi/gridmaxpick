Grid Max Pick
-------------

Is a tool to pick along max amplitudes on grid-files, initially it reads x y z files, but in the future should read GMT grd files. It was developed in C and uses libcpgplot for basic graphics 2D plotting and mouse interactions. Basic commands should be typped inside the plot window. Commands that should be implemented are:

| Key | Action |
|-----|--------|
| Q   | Quit   |
| S   | Save picked segment |
| X   | Zoom in and out     |
| P   | Pick a single frequency |
| A   | Pick inside a box area  |
| D   | Delete picks inside a box area |
| .   | Creates a new pick-set  |
| ,   | Move to previous pick-set |

Compile
=======

```bash
# Run the qmake tool to prepare the makefile
% qmake
# Run the generated Makefile
% make 
# Copy the grid file
% cp somegrid.grd x.grd
# get the xyz file
% bash conv.sh
# Run the tool
% ./testgrid
```
