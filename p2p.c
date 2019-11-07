#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ncurses.h>
#include <time.h>


/*

I need to store:
	a color
	a length
for each cell
and also a list of links
for that, we'll do for each cell: can it communicate with the one on the left
and the one at the bottom
	for each communication, there is a one on two change it can
	if it can, then the one on the bottom can communicate with the one on top
	and the one on the right can communicate with the one on the left
	so we would store a struct (can comm with l u d r), actually that would be a int with a binary and (&)

so i need for each cell:
* last color (for ncurses)
* length
* link enabled with what 

* and then for each step, we check each cell
* if length < , then we update
but in python, i had a 'previous' and 'after' state list, should i need it now?
yes i do, bc otherwise each step would be full of ones
so i will have a time_shift function, that will swap the required ints (length and last color). i think i will just duplicate variables. i would have a 'enter propagate time' that will copy 'new' from 'old', then a copy, that compare cell.new with neighbour.old, and once update is done, i would have a exit_propagate (or begin/end) that will shift variables one with the other

at the end of end_propagate, if new != old, then refresh display

*/

struct cell {
	int color;
	int new_color;
	int length;
	int new_length;
	int links;
	int changed;
};

int rows;
int cols;

int color = 0;

const int link_ratio = 50; // percentage chances link
const int link_up = 0b0001;
const int link_down = 0b0010;
const int link_left = 0b0100;
const int link_right = 0b1000;

/*attron(COLOR_PAIR(1));
	mvaddstr(5, 5, "test   ");
	attroff(COLOR_PAIR(1)); */

void init(struct cell **a) {
	*a = malloc(sizeof(struct cell) * rows * cols);
	for (int i = 0; i < rows * cols; i++) { 
		(*a)[i].color = 0;
		(*a)[i].length = 0;
		(*a)[i].links = 0;
	}
}

void debug_links(struct cell *a) {
	int col, row;
	for (int i = 0; i < rows * cols; i++) {
		col = i % cols;
		row = i / cols;
		mvprintw(row, col * 4, "%d", a[row * cols + col].links);
	}
}

void create_links(struct cell *a, int force) {
	int col, row;
	for (int i = 0; i < rows * cols; i++) {
		col = i % cols;
		row = i / cols;
		a[row * cols + row].links = 0;
	}
	int up; 
	for (int i = 0; i < rows * cols; i++) {
		col = i % cols;
		row = i / cols;
		if (row < (rows - 1)) {
			up = false;

			if ((rand() % 100) < link_ratio) { 
				up = true;
				a[row * cols + col].links = a[row * cols + col].links | link_down;
				a[(row + 1) * cols + col].links = a[(row + 1) * cols + col].links | link_up;
			}
		}
		if (col < (cols - 1)) {
			if ((force & !up) |
				 ((rand() % 100) < link_ratio)) { 
				a[row * cols + col].links = a[row * cols + col].links | link_right;
				a[row * cols + col + 1].links = a[row * cols + col + 1].links | link_left;
			}
		} 
	} 
}


int redraw(struct cell *a, int row, int col) {
	int color = a[row * cols + col].color;
	attron(COLOR_PAIR(color));
	mvprintw(row, col, " ");
	attroff(COLOR_PAIR(color)); 
}

int compare(struct cell *a, int row, int col, int row2, int col2) {
	if (a[row2 * cols + col2].length > a[row * cols + col].new_length) { 
		a[row * cols + col].changed = true;
		a[row * cols + col].new_length = a[row2 * cols + col2].length;
		a[row * cols + col].new_color = a[row2 * cols + col2].color;
		return true;
	} else {
		return false;
	}
}

int propagate(struct cell *a) {
	for (int i = 0; i < rows * cols; i++) {
		a[i].new_color = a[i].color;
		a[i].new_length = a[i].length;
		a[i].changed = 0;
	}

	// while change, for each cell, compare new with neighbours, based on links
	int one_change = 0;
	int change = 1;
	int row, col;
	while (change) {
		change = 0;
		for (int i = 0; i < rows * cols; i++) {
			col = i % cols;
			row = i / cols;
			if (a[row * cols + col].links & link_up) {
				change = change | compare(a, row, col, row - 1, col);
			} 
			if (a[row * cols + col].links & link_down) {
				change = change | compare(a, row, col, row + 1, col);
			}
			if (a[row * cols + col].links & link_right) {
				change = change | compare(a, row, col, row, col + 1);
			}
			if (a[row * cols + col].links & link_left) {
				change = change | compare(a, row, col, row, col - 1);
			}
		}
		one_change = one_change | change;
	}

	for (int i = 0; i < rows * cols; i++) {
		if (a[i].changed) {
			col = i % cols;
			row = i / cols;
			a[i].color = a[i].new_color;
			a[i].length = a[i].new_length;
			redraw(a, row, col);
			a[i].changed = false;
		}
	} 
	return one_change;
}

void set_new_color(struct cell *a) {
	color = (color + 1) % 240;
	// pick a random cell, define a new color and inc length
	int i = rand() % (rows * cols);
	a[i].color = color + 16;
	a[i].length = a[i].length + 1;

	int col, row;
	col = i % cols;
	row = i / cols;
	//mvprintw(row, col, "1");
	redraw(a, row, col);
}

int main(int arg, char *argv[]) {
	struct winsize w;
	srand(time(NULL));
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	initscr();
	curs_set(0);
	noecho();
	start_color();
// init_pair(1, 0, COLOR_GREEN);
// init_pair(2, 0, COLOR_BLUE);
// init_pair(3, 0, COLOR_WHITE);
// init_pair(4, 0, COLOR_RED);
// init_pair(5, 0, COLOR_YELLOW);
// init_pair(6, 0, COLOR_CYAN);
// init_pair(7, 0, COLOR_BLACK);
	for (int i = 0; i < 256; i++) {
		init_pair(i, 0, i);
	}

	struct cell *a;
	cols = w.ws_col;
	rows = w.ws_row;
	init(&a);
	create_links(a, true);

	//a[0, 2].color=5;
	//a[4, 33].length=9;
	//mvprintw(5, 5, "test %d", a[0, 2].color);
	//mvprintw(6, 5, "test %d", a[4, 33].length);
/*	for (int i = 0; i < 1000; i++) {
		if (i % 20 == 0) {
			set_new_color(a);
		} 
		propagate(a); 
	refresh();
	} */


	int change = 1;
	int i = 0;
	while (true) {
		if (i % 10 == 0) {
			set_new_color(a);
			i = 0;
		}
		i++;
		change = propagate(a); 
		refresh();
		usleep(50000);
	}

	getchar(); 

	endwin();
	free(a); 


	return 0;
}
