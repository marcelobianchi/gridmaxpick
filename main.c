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
	int *i;
	int *j;
	float *x;
	float *y;
} PICKS;

#define NONE 0
#define GOBYROW 1
#define GOBYCOL 1

/*
 * Pix handling
 */
void pickadd(PICKS *p, float ax, float ay, int im, int jm, int replacemode) {
	int i, it = -1;

	if (replacemode == GOBYROW) {
		for(i = 0; i < p->n; i++)
			if (p->i[i] == im) {
				it = i;
				break;
			}
	} else if (replacemode == GOBYCOL) {
		for(i = 0; i < p->n; i++)
			if (p->j[i] == jm) {
				it = i;
				break;
			}
	}

	if (it == -1) {
		p->n = p->n + 1;
		p->x = realloc(p->x, sizeof(float) * p->n);
		p->y = realloc(p->y, sizeof(float) * p->n);
		p->i = realloc(p->i, sizeof(int) * p->n);
		p->j = realloc(p->j, sizeof(int) * p->n);
		it = p->n - 1;
	}

	p->x[it] = ax;
	p->y[it] = ay;
	p->i[it] = im;
	p->j[it] = jm;

	return;
}

PICKS *newpick() {
	PICKS *p = malloc(sizeof(PICKS));
	p->n = 0;

	p->x = NULL;
	p->y = NULL;
	p->i = NULL;
	p->j = NULL;

	return p;
}

/*
 * Coordinate transformation functions
 */
inline int ij2index(GRID *g, int i, int j) {
	int ll  = j*g->nx + i;
//	fprintf(stderr,"%d / %d\n", ll, g->nx * g->ny);
	return ll;
}

inline void index2ij(GRID *g, int index, int *i, int *j) {
	*j = index / g->nx;
	*i = index - *j * g->nx;
//	fprintf(stderr,"%d %d %d\n",index, *i, *j);
	return;
}

inline int ij2xy(GRID *g, int i, int j, float *x, float *y) {
	int isg = (i >= 0 && i < g->nx && j >= 0 && j < g->ny) ? 0 : 1;

	*x = g->xmin + (i * g->dx);
	*y = g->ymin + (j * g->dy);

	return isg;
}

inline int xy2ij(GRID *g, float x, float y, int *i, int *j, int limit) {
	int isg;

	float xoffset = (x < (g->xmin)) ? -0.5 : 0.5;
	float yoffset = (y < (g->ymin)) ? -0.5 : 0.5;

	*i = ((x - g->xmin) / g->dx + xoffset);
	*j = ((y - g->ymin) / g->dy + yoffset);

	if (limit) {
		if (*i < 0 ) *i = 0;
		if (*i >= g->nx) *i = (g->nx-1);

		if (*j < 0 ) *j = 0;
		if (*j >= g->ny) *j = (g->ny-1);
	}

	isg = (*i >= 0 && *i < g->nx && *j >= 0 && *j < g->ny) ? 0 : 1;

	return isg;
}

/*
 * Tools
 */
void order(float *x1, float *x2) {
	if (*x1 > *x2) {
		float aux = *x1;
		*x1 = *x2;
		*x2 = aux;
	}
	return;
}

/*
 * Operation
 */
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

	cpggray(data->values, data->nx, data->ny, 1 ,data->nx, 1, data->ny, 1, -1, tr);

	cpgsci(2);
	for(i=0; i < np; i++) {
		fprintf(stderr,"Plotting picks %d of %d points\n",i,p[i]->n);
		if (i == cp)
			cpgpt(p[i]->n, p[i]->x, p[i]->y,2);
		else
			cpgline(p[i]->n, p[i]->x, p[i]->y);
	}
	cpgsci(1);
}

int pickrow(GRID *data, int i, int js, int je) {
	int j, index;
	int maxindex = ij2index(data, i, js);
	float max = data->values[maxindex];

	for(j=js+1; j<=je; j++) {
		index = ij2index(data, i, j);
		if (data->values[index] > max) {
			max = data->values[index];
			maxindex = index;
		}
	}

	return maxindex;
}

void boxpick(GRID *data, PICKS *p, AREA *box) {
	int is,js,ie,je;
	int i;

	xy2ij(data,box->xmin, box->ymin, &is, &js, 1);
	xy2ij(data,box->xmax, box->ymax, &ie, &je, 1);

	for(i = is; i <= ie; i++) {
		float ax, ay;
		int im, jm;
		int pick;
		pick = pickrow(data, i, js, je);
		index2ij(data, pick, &im, &jm);
		ij2xy(data, im, jm, &ax, &ay);
		pickadd(p, ax, ay, im, jm, GOBYROW);
	}

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
	picks[0] = newpick();
	np = 1; cp = 0;

	// Reset all grid area
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
				picks = realloc(picks, sizeof(PICKS *) * np);
				picks[cp] = newpick();
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

		// Delete pick area
		case 'D': {
			float axx, ayy;
			axx = ax;
			ayy = ay;

			cpgsci(3);
			getonechar(&ax, &ay, 1, 1);
			cpgsci(1);

			order(&ax,&axx);
			order(&ay,&ayy);

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

			order(&ax,&axx);
			order(&ay,&ayy);

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
	return control(&data);
}
