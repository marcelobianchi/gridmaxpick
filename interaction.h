/*
    This file is part of sactools package.
    
    sactools is a package to managing and do simple processing with SAC
    seismological files.
    Copyright (C) 2012  Marcelo B. de Bianchi

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include <cpgplot.h>
#include <ctype.h>
#include <math.h>

#define WARNING 7
#define ERROR 2
#define ANOUNCE 3


/* Interaction */
extern char interaction_message[2048];

void alert(int level);
float lerfloat(char *text);
void lerchar(char *text, char *output, int max);
int lerint(char *text);
int getmouse(int a, int b, int c, int d, char *interaction_message);
char getonechar(float *axx, float *ayy, int hook, int upper);
int yesno(char *text);
