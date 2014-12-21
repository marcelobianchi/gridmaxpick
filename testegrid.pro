TEMPLATE = app
CONFIG += console
CONFIG -= qt
QMAKE_CFLAGS += -DUSEGMTGRD
LIBS += -lcpgplot -lgmt -lm
SOURCES += main.c interaction.c
