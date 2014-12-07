#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "interaction.h"

typedef struct area {
	float xmin, xmax, ymin,ymax;
} AREA;

typedef struct grid {
	float *values;
	int nx, ny;
	float xmin, xmax;
	float ymin, ymax;
	float dx, dy;
} GRID;

typedef struct picks {
	int n;
	float *x;
	float *y;
} PICKS;

void order(float *x1, float *x2) {
	if (*x1 > *x2) {
		float aux = *x1;
		*x1 = *x2;
		*x2 = aux;
	}
	return;
}

float *load(char *filename, float *xmin, float *xmax, float *dx, int *nx, float *ymin, float *ymax, float *dy, int *ny) {
	FILE *ent;
	ent = fopen(filename, "r");
	float *data;
	int i;

	fscanf(ent, "%f %f %f %d", xmin, xmax, dx, nx);
	fscanf(ent, "%f %f %f %d", ymin, ymax, dy, ny);

	int size = (*nx) * (*ny);
	data = malloc( sizeof(float) * (size));
	for(i=0;i<(size);i++) {
		fscanf(ent, "%f", &data[i]);
	}
	fclose(ent);
	return data;
}

void plot(GRID *data, PICKS **p, int np, int cp, AREA *pane) {
	int i;
	float tr[6] = { data->xmin - data->dx, data->dx, 0, data->ymin - data->dy, 0, data->dy };

	cpgeras();

	cpgenv(pane->xmin, pane->xmax, pane->ymin, pane->ymax, 0, 0);

	cpggray(data->values, data->nx, data->ny, 1 ,data->nx, 1, data->ny, 1,0, tr);

	for(i=0; i < np; i++) {
		if (cp == i)
			continue;
		cpgline(p[i]->n, p[i]->x, p[i]->y);
	}
}

void boxpick(GRID *data, PICKS *p, AREA *box) {
	return;
}

void boxunpick(GRID *data, PICKS *p, AREA *box) {
	return;
}

int control(GRID *data) {

	PICKS **picks = NULL;
	int np = 0; // Number of picks
	int cp = -1; // Current pick
	AREA panearea;
	AREA pickarea;

	// Mouse interaction
	float ax, ay;
	char ch = 'A';

	// Init
	cpgopen("/XWINDOW");
	cpgask((0));
	picks =   malloc(sizeof(PICKS *) * 1);
	picks[0] = malloc(sizeof(PICKS) * 1);
	np = 1; cp = 0;

	panearea.xmin = data->xmin - data->dx;
	panearea.xmax = data->xmax + data->dx;
	panearea.ymin = data->ymin - data->dy;
	panearea.ymax = data->ymax + data->dy;

	while (ch != 'Q') {
		/*
		 * Pre switch
		 */
		plot(data, picks, np, cp, &panearea);
		ch = getonechar(&ax, &ay, 0, 1);

		switch(ch) {

		/*
		 * Navigation
		 */

		// Zoom (Right-mouse)
		case 'X': {
			float axx, ayy;
			axx = ax;
			ayy = ay;

			cpgsci(2);
			getonechar(&ax, &ay, 1, 1);
			cpgsci(1);

			if (ax != axx && ay != ayy) {
				order(&ax,&axx);
				order(&ay,&ayy);
				panearea.xmin = ax;
				panearea.xmax = axx;
				panearea.ymin = ay;
				panearea.ymax = ayy;
			} else {
				// Reset zoom
				panearea.xmin = data->xmin - data->dx;
				panearea.xmax = data->xmax + data->dx;
				panearea.ymin = data->ymin - data->dy;
				panearea.ymax = data->ymax + data->dy;
			}
			break;
		}

		/*
		 * Segment management
		 */

		// Next segment
		case '.': {
			if (cp == (np - 1)) {
				np++;
				cp = np -1;
				picks =    realloc(picks, sizeof(PICKS *) * np);
				picks[cp] = malloc(sizeof(PICKS) * 1);
			} else
				cp++;
			break;
		}

		// Previous segment
		case ',': {
			cp --;
			if (cp < 0) cp = 0;
			break;
		}

		/*
		 * Picking commands
		 */

		// One pick
		case 'P':
			break;

		// Delete pick area
		case 'D': {
			float axx, ayy;
			axx = ax;
			ayy = ay;

			cpgsci(3);
			getonechar(&ax, &ay, 1, 1);
			cpgsci(1);

			pickarea.xmin = ax;
			pickarea.xmax = axx;
			pickarea.ymin = ay;
			pickarea.ymax = ayy;

			boxunpick(data, picks[cp], &pickarea);
			break;
		}

		// Add pick area (Left-mouse)
		case 'A': {
			float axx, ayy;
			axx = ax;
			ayy = ay;

			cpgsci(3);
			getonechar(&ax, &ay, 1, 1);
			cpgsci(1);

			pickarea.xmin = ax;
			pickarea.xmax = axx;
			pickarea.ymin = ay;
			pickarea.ymax = ayy;

			boxpick(data, picks[cp], &pickarea);
			break;
		}

		/*
		 * Quit, save
		 */

		// Save
		case 'S':
			break;

		// Quit
		case 'Q':
			break;
		}
	}

	return 0;
}

int main(void)
{
	GRID data;

	data.values = load("_D", &data.xmin, &data.xmax, &data.dx, &data.nx, &data.ymin, &data.ymax, &data.dy, &data.ny);

//	fprintf(stderr,"X: %f %f %f %d\n",data.xmin,data.xmax,data.dx,data.nx);
//	fprintf(stderr,"Y: %f %f %f %d\n",data.ymin,data.ymax,data.dy,data.ny);

	return control(&data);
}
