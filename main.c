#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include "interaction.h"
#include "colortable.h"

#define DEBUG   0    // Enable debuging
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

void status(char *message) {
	cpgsci(2);
	cpgsch(0.75);
	cpgmtxt("B",4.5, 0.5, 0.5, message);
	cpgsci(1);
	cpgsch(1.0);
}

/*
 * Pix handling
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
 * Coordinate transformation functions
 */
inline int ij2index(GRID *g, int i, int j) {
	int ll  = j*g->nx + i;
	// fprintf(stderr,"%d / %d\n", ll, g->nx * g->ny);
	return ll;
}

inline void index2ij(GRID *g, int index, int *i, int *j) {
	*j = index / g->nx;
	*i = index - *j * g->nx;
	// fprintf(stderr,"%d %d %d\n",index, *i, *j);
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

void plot(GRID *data, PICKS **p, int np, int cp, AREA *pane) {
	int i;
	float tr[6] = { data->xmin - data->dx, data->dx, 0, data->ymin - data->dy, 0, data->dy };
	char message[1024];

	cpgeras();

	cpgenv(pane->xmin, pane->xmax, pane->ymin, pane->ymax, 0, 0);

	cpgimag(data->values, data->nx, data->ny, 1, data->nx, 1, data->ny, 0, 1, tr);

	strcpy(message,"Picks: ");
	cpgsci(0);
	for(i=0; i < np; i++) {
		if (i == cp) {
			cpgpt(p[i]->n, p[i]->x, p[i]->y,2);
			sprintf(message,"%s [%03d]",message,i);
		} else {
			cpgline(p[i]->n, p[i]->x, p[i]->y);
			sprintf(message,"%s  %03d ",message,i);
		}
	}
	cpgsci(1);

	cpgsch(1.3);
	cpgmtxt("T",2.1, 0.5, 0.5, "GridPick Code");

	cpgsch(0.8);
	cpgmtxt("T", 1.1, 0.0, 0.0, message);
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

PICKS *loadpicks(char *filename, int *np, int *cp) {
	return NULL;
}

int savepicks(char *filename, PICKS **p, int np, int cp) {
	FILE *sai;
	int i, j;

	sai = fopen(filename,"w");
	if (sai == NULL) {
		sprintf(interaction_message, "Cannot write to file %s", filename);
		alert(1);
		return 1;
	}

	for(i = 0; i < np; i++) {
		fprintf(sai, "> %d %d %c\n",i, p[i]->n, (cp == i) ? 'S' : 'U');
		for(j = 0; j < p[i]->n; j++)
			fprintf(sai, "%f %f %d %d\n"
					,p[i]->x[j]
					,p[i]->y[j]
					,p[i]->i[j]
					,p[i]->j[j]);
	}

	fclose(sai);

	return 0;
}

int control(char *gridfilename, char *pickfilename) {

	GRID data;                       // Holds the grid data

	PICKS **picks = NULL;            // Pickset storage area
	int np = 0;                      // Number of picks
	int cp = -1;                     // Current pick

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
	data.values = load(gridfilename, &data.xmin, &data.xmax, &data.dx, &data.nx, &data.ymin, &data.ymax, &data.dy, &data.ny);

	/*
	 * Init
	 */
	q_count = 0;
	picks =   malloc(sizeof(PICKS *) * 1);
	picks[0] = newpick();
	np = 1; cp = 0;
	save_is_needed = 1;

	/*
	 * Reset all grid area
	 */
	panearea.xmin = data.xmin - data.dx;
	panearea.xmax = data.xmax + data.dx;
	panearea.ymin = data.ymin - data.dy;
	panearea.ymax = data.ymax + data.dy;


	/*
	 * Color table Dump
	 */
	int cs, ce;
	int i;
	float r,g,b, nr, ng, nb;

	cpgqcir(&cs, &ce);
	if (DEBUG) fprintf(stderr,"cs: %d ce: %d\n",cs,ce);

	cpgscir(cs, cs + n_colortable - 1);
	cpgqcir(&cs, &ce);
	if (DEBUG) fprintf(stderr,"cs: %d ce: %d\n",cs,ce);

	for(i = cs; i <= ce; i++) {

		nr = colortable[i-cs][0] / 255.0;
		ng = colortable[i-cs][1] / 255.0;
		nb = colortable[i-cs][2] / 255.0;

		cpgqcr(i, &r, &g, &b);
		if (DEBUG) fprintf(stderr," %d %.2f %.2f %.2f -> %.2f %.2f %.2f\n", i, r, g, b, nr, ng, nb);
		cpgscr(i, nr, ng, nb);
	}

	while (ch != 'Q') {
		/*
		 * Pre switch
		 */
		plot(&data, picks, np, cp, &panearea);
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
			if (cp == (np - 1)) {
				if (np == MAXNP) {
					sprintf(interaction_message, "Cannot add more than %d picksets.", MAXNP);
					alert(WARNING);
					break;
				}
				np++;
				cp = np -1;
				picks = realloc(picks, sizeof(PICKS *) * np);
				picks[cp] = newpick();
				save_is_needed = 1;
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

			boxunpick(&data, picks[cp], &pickarea);

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

			boxpick(&data, picks[cp], &pickarea);

			save_is_needed = 1;
			break;
		}

		case 'Z':
		case 'Y': {
			int is, js;
			xy2ij(&data, ax, ay, &is, &js, 1);
			pickadd(picks[cp], ax, ay, is, js, GOBYROW);
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
			if (pickfilename == NULL) {
				lerchar("Enter the filename for saving the picks:", output_pick_filename, 1024);
				pickfilename = output_pick_filename;
			}
			save_is_needed = savepicks(pickfilename, picks, np, cp);
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
	char *gridfilename = NULL;
	char *pickfilename = NULL;

	/*
	 * Parse command line
	 */
	if (argc < 2) {
		fprintf(stderr,"Invalid call, expected:\n%s <Grid> [pickfile]\n", argv[0]);
		return 1;
	}

	/*
	 * Init code
	 */
	cpgopen("/XWINDOW");
	cpgask((0));

	/*
	 * Parse the command-line argument
	 */
	gridfilename = argv[1];
	pickfilename = (argc > 2) ? argv[2] : NULL;

	/*
	 * Run the code
	 */
	return control(gridfilename, pickfilename);
}
