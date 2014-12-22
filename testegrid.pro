TEMPLATE = app
CONFIG += console
CONFIG -= qt
QMAKE_CFLAGS += -DUSEGMTGRD -I/usr/include/gmt/
LIBS += -lcpgplot -lgmt -lm
SOURCES += main.c interaction.c
