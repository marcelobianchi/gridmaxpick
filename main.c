#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "interaction.h"
#include "colortable.h"

#ifdef USEGMTGRD
#include <gmt/gmt.h>
#endif

#define DEBUG   1    // Enable debuging
#define MAXNP  16    // Max number of pickset allowed
#define NONE    0    // Picking mode (in the future), NONE
#define GOBYROW 1    // pick ROW wise
#define GOBYCOL 1    // pick COL wise

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

typedef struct workpackage {
	char *pickfilename;
	char *gridfilename;

	char *sta, *stb;
	float distance;

	PICKS **picks;
	int np, cp;
} WORKPACKAGE;

/*
 * General support methods
 */
void status(char *message) {
	cpgsci(2);
	cpgsch(0.75);
	cpgmtxt("B",4.5, 0.5, 0.5, message);
	cpgsci(1);
	cpgsch(1.0);
}

char * makecopy(char *source) {
	char *destination = malloc(sizeof(char) * (strlen(source) + 1) );
	strcpy(destination, source);
	return destination;
}

void order(float *x1, float *x2) {
	if (*x1 > *x2) {
		float aux = *x1;
		*x1 = *x2;
		*x2 = aux;
	}
	return;
}

/*
 * Picks handling
 */
void pickadd(PICKS *p, float ax, float ay, int im, int jm, int replacemode) {
	int i, it = -1;
	int insert;

	/*
	 * Append
	 */
	it = p->n;
	insert = 1;

	if (replacemode == GOBYROW) {
		for(i = 0; i < p->n; i++)
			if (p->i[i] == im) {
				/*
				 * Replace
				 */
				it = i;
				insert = 0;
				break;
			} else if (p->i[i] > im) {
				/*
				 * Insert in the middle
				 */
				it = i;
				insert = 1;
				break;
			}
	} else if (replacemode == GOBYCOL) {
		for(i = 0; i < p->n; i++)
			if (p->j[i] == jm) {
				it = i;
				break;
			} else if (p->j[i] > jm) {
				it = i;
				insert = 1;
				break;
			}
	}

	if (insert) {
		p->n = p->n + 1;
		p->x = realloc(p->x, sizeof(float) * p->n);
		p->y = realloc(p->y, sizeof(float) * p->n);
		p->i = realloc(p->i, sizeof(int) * p->n);
		p->j = realloc(p->j, sizeof(int) * p->n);

		int amount = p->n - it -1;
		if (amount > 0) {
			memcpy(&p->i[it+1], &p->i[it], sizeof(int) * amount);
			memcpy(&p->j[it+1], &p->j[it], sizeof(int) * amount);
			memcpy(&p->x[it+1], &p->x[it], sizeof(float) * amount);
			memcpy(&p->y[it+1], &p->y[it], sizeof(float) * amount);
		}

		if (DEBUG) fprintf(stderr,"Insert (it=%d n=%d amount=(%d)%d\n",it, p->n-1, p->n, amount);
	}

	p->x[it] = ax;
	p->y[it] = ay;
	p->i[it] = im;
	p->j[it] = jm;

	return;
}

void pickdel(PICKS *p, int from, int to) {
	int amount;
	int moveamount;

	/*
	 * Compute cut amount
	 */
	amount = to - from + 1;

	/*
	 * Comput move amount
	 */
	moveamount = p->n - to - 1;

	/*
	 * Cut if necessary
	 */
	if (to >= from) {

		/*
		 * Move if necessary
		 */
		if (moveamount > 0) {
			memmove(&p->i[from], &p->i[to+1], sizeof(int) * (moveamount));
			memmove(&p->j[from], &p->j[to+1], sizeof(int) * (moveamount));
			memmove(&p->x[from], &p->x[to+1], sizeof(float) * (moveamount));
			memmove(&p->y[from], &p->y[to+1], sizeof(float) * (moveamount));
		}

		/*
		 * Real pick vectors cut
		 */
		p->n = p->n - amount;
		p->x = realloc(p->x, sizeof(float) * p->n);
		p->y = realloc(p->y, sizeof(float) * p->n);
		p->i = realloc(p->i, sizeof(int) * p->n);
		p->j = realloc(p->j, sizeof(int) * p->n);

		/*
		 * Do we want report ?
		 */
		if (DEBUG) fprintf(stderr,"Cutted pick from %d to %d amount %d rested %d movedamount %d\n",from, to, amount, p->n, moveamount);
	}
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
 * IO
 */

float *loadASCII(char *filename, float *xmin, float *xmax, float *dx, int *nx, float *ymin, float *ymax, float *dy, int *ny) {

	FILE *ent;
	float *data;
	int i;

	ent = fopen(filename, "r");
	if (ent == NULL) {
		sprintf(interaction_message, "File %s cannot be open.", filename);
		alert(1);

		return NULL;
	}

	fscanf(ent, "%f %f %f %d", xmin, xmax, dx, nx);
	fscanf(ent, "%f %f %f %d", ymin, ymax, dy, ny);

	int size = (*nx) * (*ny);
	data = malloc( sizeof(float) * (size));
	for(i=0;i<(size);i++)
		fscanf(ent, "%f", &data[i]);
	fclose(ent);

	return data;
}

#ifdef USEGMTGRD
float *loadGRD(char *filename, float *xmin, float *xmax, float *dx, int *nx, float *ymin, float *ymax, float *dy, int *ny) {
	struct GRD_HEADER grd;
	float *a = NULL;
	GMT_LONG nm;

	if (DEBUG) fprintf(stderr,"Loading (GMT/GRD) file: %s\n", filename);

	GMT_grd_init (&grd, 0, NULL, FALSE);
	GMT_read_grd_info (filename, &grd);

	*xmin = grd.x_min;
	*xmax = grd.x_max;
	*dx = grd.x_inc;
	*nx = grd.nx;

	*ymin = grd.y_min;
	*ymax = grd.y_max;
	*dy = grd.y_inc;
	*ny = grd.ny;

	if (DEBUG) fprintf(stderr,"xMin: %f xMax: %f dX: %f nX: %d\n", *xmin, *xmax, *dx, *nx);
	if (DEBUG) fprintf(stderr,"yMin: %f yMax: %f dY: %f nY: %d\n", *ymin, *ymax, *dy, *ny);

	nm = GMT_get_nm (grd.nx, grd.ny);
	a = (float *) malloc (sizeof(float) * nm);
	GMT_read_grd (filename, &grd, a, 0.0, 0.0, 0.0, 0.0, GMT_pad, FALSE);
	if (DEBUG) fprintf(stderr,"Total of %ld data points\n", nm);

	return a;
}
#endif

int loadPKFILE(WORKPACKAGE *wp) {
	int natt;               // Number of readen attributes
	float distance;         // Distance from top PICK line
	char sta[20], stb[20];  // Station A and B from top PICK line

	/* Pick File Example:
	 *
	 * > <sta> <stb> <Distance>
	 * > <seq> <n-points> <U|S>
	 * X Y I J Period Frequency <sta> <stb> <Distance>
	 * > <seq> <n-points> <U|S>
	 *
	 */

	// Does we have a file?
	if (wp->pickfilename == NULL) return 1;

	// Open it
	FILE *ent;
	if ( (ent = fopen(wp->pickfilename, "r")) == NULL) {
		fprintf(stderr, "Cannot read pickfile %s\n", wp->pickfilename);
		return 1;
	}

	// Read the top line
	natt = fscanf(ent,"> %19s %19s %f\n", sta, stb, &distance);
	if (natt != 3) {
		fprintf(stderr, "Is this (%s) a PICK file ? First line missmatch\n", wp->pickfilename);
		return 1;
	}

	// If user does not supplied values in CMD line
	if (wp->sta == NULL) wp->sta = makecopy(sta);
	if (wp->stb == NULL) wp->stb = makecopy(stb);
	if (wp->distance == -1234.5) wp->distance = distance;


	/*
	 * Line Picking stuff
	 */
	char lsta[20], lstb[20];
	float ld, x, y, f, p;
	int i,j;

	/*
	 * Header line stuff
	 */
	int seq, np;
	char sel;

	// Starting reading process
	char line[2000];
	int lineseq = 0;
	int picksel = 0;

	fgets(line, 1999, ent);
	while (!feof(ent)) {
		if (line[0] == '>') {
			/*
			 * Header line
			 */
			natt = sscanf(line,"> %d %d %c\n",&seq, &np, &sel);
			if (natt != 3) {
				fprintf(stderr, "Segment description is invalid.\n");
				break;
			}

			// Check sequence
			if (seq != lineseq) {
				fprintf(stderr, "Line-sequence does not match.\n");
				break;
			}

			// Check pick active flag
			if (sel != 'U' && sel != 'S') {
				fprintf(stderr, "Invalid header line flag.\n");
				break;
			}

			// Is this the active one ?
			if (sel == 'S') picksel = seq;

			/*
			 * Point line
			 */
			PICKS *pickdata = newpick();

			for(; np > 0; np--) {
				natt = fscanf(ent,"%f %f %d %d %f %f %s %s %f\n",&x, &y, &i, &j, &f, &p, lsta, lstb, &ld);

				// Check number of items in line
				if (natt != 9) {
					fprintf(stderr, "Wrong number of items on line decode.");
					break;
				}

				// Check station codes
				if (strncmp(sta, lsta, strlen(sta)) != 0) {
					fprintf(stderr, "Station A (%s) does not match (%s).\n",lsta, sta);
					break;
				}

				if (strncmp(stb, lstb, strlen(stb)) != 0) {
					fprintf(stderr, "Station B (%s) does not match.(%s) \n",lstb, stb);
					break;
				}

				// Check distances
				if (ld != distance) {
					fprintf(stderr, "Distance does not match.\n");
					break;
				}

				// Finally add it on !
				pickadd(pickdata, x, y, i, j, GOBYROW);
			}

			if (np == 0) {
				// We add if, all points were OK !
				wp->np++;
				wp->cp = wp->np -1;
				wp->picks = realloc(wp->picks, sizeof(PICKS *) * wp->np);
				wp->picks[wp->cp] = pickdata;
			}

			// Wait for the next sequence segment
			lineseq++;
		} else {
			if (strlen(line) > 0)
				fprintf(stderr, "Ignored line, %s\n",line);
		}

		// Get the next record
		fgets(line, 1999, ent);
	}

	if (picksel < wp->np)
		wp->cp = picksel;

	fclose(ent);
	return 0;
}

int savePKFILE(WORKPACKAGE *wp) {
	FILE *sai;
	int i, j;

	sai = fopen(wp->pickfilename,"w");
	if (sai == NULL) {
		sprintf(interaction_message, "Cannot write to file %s", wp->pickfilename);
		alert(1);
		return 1;
	}

	fprintf(sai, "> %s %s %f\n", wp->sta, wp->stb, wp->distance);
	for(i = 0; i < wp->np; i++) {
		fprintf(sai, "> %d %d %c\n", i, wp->picks[i]->n, (wp->cp == i) ? 'S' : 'U');
		for(j = 0; j < wp->picks[i]->n; j++)
			fprintf(sai, "%f %f %d %d 0 0 %s %s %f\n"
					,wp->picks[i]->x[j]
					,wp->picks[i]->y[j]
					,wp->picks[i]->i[j]
					,wp->picks[i]->j[j]
					,wp->sta
					,wp->stb
					,wp->distance);
	}

	fclose(sai);

	return 0;
}

/*
 * Coordinate transformation functions
 */
inline int ij2index(GRID *g, int i, int j) {
	int ll  = j*g->nx + i;
	// fprintf(stderr,"%d / %d\n", ll, g->nx * g->ny);
	return ll;
}

inline void index2ij(GRID *g, int index, int *i, int *j) {
	int jtemp = index / g->nx;
	int itemp = index - jtemp * g->nx;

	if (i != NULL) *i = itemp;
	if (j != NULL) *j = jtemp;

	// fprintf(stderr,"%d %d %d\n",index, *i, *j);
	return;
}

inline int ij2xy(GRID *g, int i, int j, float *x, float *y) {
	int isg = (i >= 0 && i < g->nx && j >= 0 && j < g->ny) ? 0 : 1;

	if (x != NULL)
		*x = g->xmin + (i * g->dx);

	if (y != NULL)
		*y = g->ymin + (j * g->dy);

	return isg;
}

inline int xy2ij(GRID *g, float x, float y, int *i, int *j, int limit) {
	int isg;

	int itemp, jtemp;

	float xoffset = (x < (g->xmin)) ? -0.5 : 0.5;
	float yoffset = (y < (g->ymin)) ? -0.5 : 0.5;

	itemp = ((x - g->xmin) / g->dx + xoffset);
	jtemp = ((y - g->ymin) / g->dy + yoffset);

	if (limit) {
		if (itemp < 0 ) itemp = 0;
		if (itemp >= g->nx) itemp = (g->nx-1);

		if (jtemp < 0 ) jtemp = 0;
		if (jtemp >= g->ny) jtemp = (g->ny-1);
	}

	isg = (itemp >= 0 && itemp < g->nx && jtemp >= 0 && jtemp < g->ny) ? 0 : 1;

	if (i != NULL) *i = itemp;
	if (j != NULL) *j = jtemp;

	return isg;
}

/*
 * Operational
 */

void plot(GRID *data, WORKPACKAGE *wp, AREA *pane) {
	int i;
	float tr[6] = { data->xmin - data->dx, data->dx, 0, data->ymin - data->dy, 0, data->dy };
	char message[1024];

	cpgeras();

	cpgenv(pane->xmin, pane->xmax, pane->ymin, pane->ymax, 0, 0);

	cpgimag(data->values, data->nx, data->ny, 1, data->nx, 1, data->ny, 0, 1, tr);

	strcpy(message,"Picks: ");
	cpgsci(0);
	for(i=0; i < wp->np; i++) {
		if (i == wp->cp) {
			cpgpt(wp->picks[i]->n, wp->picks[i]->x, wp->picks[i]->y,2);
			sprintf(message,"%s [%03d]",message,i);
		} else {
			cpgline(wp->picks[i]->n, wp->picks[i]->x, wp->picks[i]->y);
			sprintf(message,"%s  %03d ",message,i);
		}
	}
	cpgsci(1);

	cpgsch(0.8);
	cpgmtxt("T", 1.1, 0.0, 0.0, message);

	sprintf(message, "Station] %s to Station] %s Distance] %.2f km", wp->sta, wp->stb, wp->distance);
	cpgmtxt("R", 1.1, 0.0, 0.0, message);

	cpgsch(1.3);
	cpgmtxt("T",2.1, 0.5, 0.5, "GridPick Code");

	cpgsch(1.0);
}

void showhelp() {
	float p, py;

	cpgsch(0.75);

	cpgsvp(0.35, 0.95, 0.05, 0.55);
	cpgswin(0.0,1.0,0.0,1.0);
	cpgrect(0.0,1.0,0.0,1.0);
	cpgsci(0);
	cpgrect(0.01,.99,0.01,0.99);
	cpgsci(1);


	py = 0.02;
	cpgsci(5);
	cpgsch(1.0);
	p = -1.0; cpgmtxt("T", p, py, 0.0, "General Controls:");

	cpgsci(1);
	p -= 2.0; cpgmtxt("T", p, py, 0.0, "s - Save Picks");
	p -= 1.0; cpgmtxt("T", p, py, 0.0, "h - Help Message");
	p -= 1.0; cpgmtxt("T", p, py, 0.0, "q - Quit program");
	p -= 1.0; cpgmtxt("T", p, py, 0.0, "x - Zoom In/Out (right mouse)");

	cpgsci(5);
	p -= 2.0; cpgmtxt("T", p, py, 0.0, "Pick Set Control:");

	cpgsci(1);
	p -= 2.0; cpgmtxt("T", p, py, 0.0, ", - Previous PickSet");
	p -= 1.0; cpgmtxt("T", p, py, 0.0, ". - Next PickSet / Add PickSet");
	p -= 1.0; cpgmtxt("T", p, py, 0.0, "d - Auto Delete pick points by region");
	p -= 1.0; cpgmtxt("T", p, py, 0.0, "a - Auto Add pick points by region (left mouse)");
	p -= 1.0; cpgmtxt("T", p, py, 0.0, "z - Manual Pick (y also works - german keyboard)");


	p -= 2.0; cpgmtxt("T", p, py, 0.0, "  --  Any Key to close this help message !");
	getonechar(NULL, NULL, 0, 0);
	/*
	 * Reset to return
	 */
	cpgsch(1.0);
	cpgsci(1);

	return;
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

	/*
	 * Convert picks to grid index
	 */
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
	int is,js,ie,je, i;
	int from, to;

	/*
	 * Convert picks to grid index
	 */
	xy2ij(data,box->xmin, box->ymin, &is, &js, 1);
	xy2ij(data,box->xmax, box->ymax, &ie, &je, 1);

	/*
	 * Multiple cut based on other index to allow nice D
	 */
	to = -1;
	for(i = p->n - 1; i >= 0; i--) {

		/*
		 * Check cut-end
		 */
		if ((p->i[i] >= is && p->i[i] <= ie) && (p->j[i] >= js && p->j[i] <= je)) {
			to = i;

			/*
			 * Search for the cut-start
			 */
			for(from = i; from >= 0; from--) {
				if (p->i[from] >= is && (p->j[from] >= js && p->j[from] <= je)) continue;
				break;
			}
			from ++;

			/*
			 * Cut
			 */
			pickdel(p, from, to);

			/*
			 * We reset to re-start process since index changed
			 */
			i = p->n;
		}
	}

	return;
}

int control(WORKPACKAGE *wp) {

	GRID data;                       // Holds the grid data

	AREA panearea;                   // Zoom area
	AREA pickarea;                   // Area for picking/unpicks

	int q_count;                     // quit hit count to quit without saving
	int save_is_needed;              // Flag to indicate that pickset is not saved
	char output_pick_filename[1024]; // When pickfilename is NULL

	float ax, ay;                    // Mouse pick variable X, Y
	char ch = ' ';                   // Mouse pick variable char

	/*
	 * Load grid file
	 */
#ifdef USEGMTGRD
	data.values = loadGRD(wp->gridfilename, &data.xmin, &data.xmax, &data.dx, &data.nx, &data.ymin, &data.ymax, &data.dy, &data.ny);
#else
	data.values = loadASCII(wp->gridfilename, &data.xmin, &data.xmax, &data.dx, &data.nx, &data.ymin, &data.ymax, &data.dy, &data.ny);
#endif

	/*
	 * Init
	 */
	q_count = 0;

	if (wp->picks == NULL) {
		wp->picks =   malloc(sizeof(PICKS *) * 1);
		wp->picks[0] = newpick();
		wp->np = 1; wp->cp = 0;
		save_is_needed = 1;
	}

	/*
	 * Reset all grid area
	 */
	panearea.xmin = data.xmin - data.dx;
	panearea.xmax = data.xmax + data.dx;
	panearea.ymin = data.ymin - data.dy;
	panearea.ymax = data.ymax + data.dy;

	/*
	 * Color table load
	 */
	int cs, ce;
	int i;
	float r,g,b, nr, ng, nb;

	cpgqcir(&cs, &ce);
	if (DEBUG) fprintf(stderr,"cs: %d ce: %d\n",cs,ce);

	cpgscir(cs, cs + n_seis_colortable - 1);
	cpgqcir(&cs, &ce);
	if (DEBUG) fprintf(stderr,"cs: %d ce: %d\n",cs,ce);

	for(i = cs; i <= ce; i++) {

		nr = seis_colortable[i-cs][0] / 255.0;
		ng = seis_colortable[i-cs][1] / 255.0;
		nb = seis_colortable[i-cs][2] / 255.0;

		cpgqcr(i, &r, &g, &b);
		if (DEBUG) fprintf(stderr," %d %.2f %.2f %.2f -> %.2f %.2f %.2f\n", i, r, g, b, nr, ng, nb);
		cpgscr(i, nr, ng, nb);
	}

	while (ch != 'Q') {
		/*
		 * Pre switch
		 */
		plot(&data, wp, &panearea);
		ch = getonechar(&ax, &ay, 0, 1);
		if (ch != 'Q') q_count = 0;

		switch(ch) {

		/*
		 * Navigation
		 */

		// Zoom (Right-mouse)
		case 'X': {
			float axx, ayy;
			axx = ax;
			ayy = ay;

			status("Pick the second point to zoom");
			cpgsci(2);
			getonechar(&ax, &ay, 1, 1);
			cpgsci(1);

			if (ax != axx && ay != ayy) {
				/*
				 * Zoom in
				 */
				order(&ax,&axx);
				order(&ay,&ayy);
				panearea.xmin = ax;
				panearea.xmax = axx;
				panearea.ymin = ay;
				panearea.ymax = ayy;
			} else {
				/*
				 * Reset zoom
				 */
				panearea.xmin = data.xmin - data.dx;
				panearea.xmax = data.xmax + data.dx;
				panearea.ymin = data.ymin - data.dy;
				panearea.ymax = data.ymax + data.dy;
			}
			break;
		}

		/*
		 * Segment management
		 */

		// Next segment
		case '.': {
			if (wp->cp == (wp->np - 1)) {
				if (wp->np == MAXNP) {
					sprintf(interaction_message, "Cannot add more than %d picksets.", MAXNP);
					alert(WARNING);
					break;
				}
				wp->np++;
				wp->cp = wp->np -1;
				wp->picks = realloc(wp->picks, sizeof(PICKS *) * wp->np);
				wp->picks[wp->cp] = newpick();
				save_is_needed = 1;
			} else
				wp->cp++;
			break;
		}

		// Previous segment
		case ',': {
			wp->cp --;
			if (wp->cp < 0) wp->cp = 0;
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

			status("Pick the second point to define the delete area");
			cpgsci(3);
			getonechar(&ax, &ay, 1, 1);
			cpgsci(1);

			order(&ax,&axx);
			order(&ay,&ayy);

			pickarea.xmin = ax;
			pickarea.xmax = axx;
			pickarea.ymin = ay;
			pickarea.ymax = ayy;

			boxunpick(&data, wp->picks[wp->cp], &pickarea);

			save_is_needed = 1;
			break;
		}

		// Add pick area (Left-mouse)
		case 'A': {
			float axx, ayy;
			axx = ax;
			ayy = ay;

			status("Pick the second point to define the auto-pick area");
			cpgsci(3);
			getonechar(&ax, &ay, 1, 1);
			cpgsci(1);

			order(&ax,&axx);
			order(&ay,&ayy);

			pickarea.xmin = ax;
			pickarea.xmax = axx;
			pickarea.ymin = ay;
			pickarea.ymax = ayy;

			boxpick(&data, wp->picks[wp->cp], &pickarea);

			save_is_needed = 1;
			break;
		}

		case 'Z':
		case 'Y': {
			int is, js;
			xy2ij(&data, ax, ay, &is, &js, 1);
			ij2xy(&data, is, js, &ax, NULL);
			pickadd(wp->picks[wp->cp], ax, ay, is, js, GOBYROW);
			break;
		}
		/*
		 * Quit, save, help
		 */

		// Help
		case 'H': {
			showhelp();
			break;
		}

		// Save
		case 'S': {
			if (wp->pickfilename == NULL) {
				lerchar("Enter the filename for saving the picks:", output_pick_filename, 1024);
				wp->pickfilename = output_pick_filename;
			}
			save_is_needed = savePKFILE(wp);
			break;
		}

		// Quit
		case 'Q': {
			if (q_count == 0 && save_is_needed == 1) {
				strcpy(interaction_message, "Picksets are not saved, Q one more time to quit.");
				alert(1);
				ch = ' ';
				q_count = 1;
			}
			break;
		}

		}
	}

	return 0;
}

int main(int argc, char **argv) {
	int i;

	WORKPACKAGE wp;

	/*
	 * Init working package
	 */
	wp.distance = -1234.5;
	wp.gridfilename = NULL;
	wp.pickfilename = NULL;
	wp.sta = NULL;
	wp.stb = NULL;
	wp.picks = NULL;
	wp.cp = 0;
	wp.np = 0;

	/*
	 * Command line:
	 *
	 * testgrid  -g x_01_08.grd [-p pick.dat] [-sa 01] [-sb 02] [-d 1200]
	 *
	 */

	for(i = 1; i < argc; i++) {
		if (strncmp("-sa",argv[i],3) == 0 && argc > i + 1) {
			wp.sta = makecopy(argv[++i]); continue;
		}

		if (strncmp("-sb",argv[i],3) == 0 && argc > i + 1) {
			wp.stb = makecopy(argv[++i]); continue;
		}

		if (strncmp("-d",argv[i],2) == 0 && argc > i + 1) {
			wp.distance = atof(argv[++i]); continue;
		}

		if (strncmp("-g",argv[i],2) == 0 && argc > i + 1) {
			wp.gridfilename = makecopy(argv[++i]); continue;
		}

		if (strncmp("-p",argv[i],2) == 0 && argc > i + 1) {
			wp.pickfilename = makecopy(argv[++i]); continue;
		}
	}

	if (wp.pickfilename != NULL) {
		fprintf(stderr, "Read? %d",
				loadPKFILE(&wp));
	}

	int stop = 0;

	if (wp.sta == NULL || wp.stb == NULL) {
		fprintf(stderr, "Need the station names.\n");
		stop = 1;
	}

	if (wp.distance == -1234.5) {
		fprintf(stderr, "Need distance.\n");
		stop = 1;
	}

	if (wp.gridfilename == NULL) {
		fprintf(stderr, "Need gridfile.\n");
		stop = 1;
	}

	if (stop) return 1;

#ifdef USEGMTGRD
	/*
	 * Initialize GMT library for use
	 */
	argc = (int)GMT_begin (argc, argv);
#endif

	/*
	 * Init code
	 */
	cpgopen("/XWINDOW");
	cpgask((0));

	/*
	 * Run the code
	 */
	return control(&wp);
}
