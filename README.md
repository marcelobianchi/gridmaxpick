Grid Max Pick
-------------

Is a tool to pick along max amplitudes on grid-files, that reads GMT grd files. It was developed in C and uses libcpgplot for basic graphics 2D plotting and mouse interactions. Basic commands should be typped inside the plot window. Commands that should be implemented are:

| Key | Action |
|-----|--------|
| Q   | Quit   |
| H   | Help   |
| S   | Save picked segment |
| X   | Zoom in and out     |
| Z|Y | Pick a single frequency |
| A   | Pick inside a box area  |
| D   | Delete picks inside a box area |
| .   | Creates a new pick-set  |
| ,   | Move to previous pick-set |

So far this app. was only used to pick surface waves dispersion curves between stations. So in the cmd-line you should inform the name of station A, station B (to compute dispersion) and distance between stations. This information is saved/restored from the pick file.

Compile
=======

```bash
# Run the qmake tool to prepare the makefile
% qmake
# Run the generated Makefile
% make
# Run the tool
% ./testgrid -g <grid> -p <pick-file> -sa <Station a> -sb <Station b> -d <Distance>
```
