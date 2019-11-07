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


// i need to do the catching up properly, not use a length, and find a way to compare which is the best.
// the catch_up begins to be mandatory. or i could base everythg on length and max_length, and max_length would be reset, but during reset, everythg would sucks
// so i def need a proper way. like old_value = value - 20 % max_col, somethg

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

int colors[2000];
int max_col;
int color;

const int link_ratio = 50; // percentage chances link
const int link_up = 0b0001;
const int link_down = 0b0010;
const int link_left = 0b0100;
const int link_right = 0b1000;

void init_colors() {
	for (int i = 0; i < 256; i++) {
		init_pair(i, 0, i);
	}
	color = 0;

	max_col = 0;
	for (int red = 0; red < 6; red++) {
		for (int green = 0; green < 6; green++) {
			for (int blue = 0; blue < 6; blue++) {
				colors[max_col] = 16 + red * 36 + green * 6 + blue;
				max_col++;
			}
			if ((red < 6) | (green < 6)) {
				for (int blue = 5; blue >= 0; blue--) {
					colors[max_col] = 16 + red * 36 + green * 6 + blue;
					max_col++;
				} 
			}
		}
	}
	for (int white = 255; white >= 232; white--) {
		colors[max_col] = white;
		max_col++;
	}
}

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

int generate_link_ratio() {
	return rand() % 30 + 30;
} 

void create_links(struct cell *a, int link_ratio) {
	int col, row;
	for (int i = 0; i < rows * cols; i++) {
		col = i % cols;
		row = i / cols;
		a[row * cols + col].links = 0;
	}

	for (int i = 0; i < rows * cols; i++) {
		col = i % cols;
		row = i / cols;
		if (row < (rows - 1)) {
			if (((rand() % 100) < link_ratio)) { 
				a[row * cols + col].links = a[row * cols + col].links | link_down;
				a[(row + 1) * cols + col].links = a[(row + 1) * cols + col].links | link_up;
			}
		}
		if (col < (cols - 1)) {
			if (((rand() % 100) < link_ratio)) { 
				a[row * cols + col].links = a[row * cols + col].links | link_right;
				a[row * cols + col + 1].links = a[row * cols + col + 1].links | link_left;
			}
		} 
	} 
}

int redraw(struct cell *a, int row, int col) {
	int color = colors[a[row * cols + col].color];
	attron(COLOR_PAIR(color));
	mvprintw(row, col, " ");
	attroff(COLOR_PAIR(color)); 
}



int catch_up(struct cell *a) {
	for (int i = 0; i < rows * cols; i++) {
		if (abs(color - a[i].color) > 20) {
			a[i].color = (color - 1) % max_col;
			redraw(a, i / cols, i % cols);
		}
	}
}

int compare(struct cell *a, int row, int col, int row2, int col2) {
	int next_cell_length = a[row2 * cols + col2].length;
	int my_length = a[row * cols + col].new_length;
	if ((my_length < next_cell_length) |
		(my_length > next_cell_length + max_col / 2)) {
		a[row * cols + col].changed = true;
		a[row * cols + col].new_length = a[row2 * cols + col2].length % 1000;
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
	color = (color + 1) % max_col;
	// pick a random cell, define a new color and inc length
	int i = rand() % (rows * cols);
	a[i].color = color;
	a[i].length = (a[i].length + 1) % max_col;

	int col, row;
	col = i % cols;
	row = i / cols;
	//mvprintw(row, col, "1");
	redraw(a, row, col);
}

void test() {
	initscr();
	curs_set(0);
	noecho();
	start_color(); // ncurses
	init_colors(); // my stuff
	for (int i = 0; i < max_col; i++) {
		attron(COLOR_PAIR(colors[i]));
		mvprintw(i / 80, i % 80, "a");
		attroff(COLOR_PAIR(colors[i]));
	}
	refresh();
	getchar();
	endwin(); 
}

void run() {
	struct winsize w;
	srand(time(NULL));
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
	initscr();
	curs_set(0);
	noecho();
	start_color();
	init_colors();
	struct cell *a;
	cols = w.ws_col - 2;
	rows = w.ws_row - 2;
	init(&a);
	//create_links(a);

	int change = 1;
	int i = 0;

	int freq_col = 20;
	int freq_link = 100;

	while (true) {
		if (i % freq_col == 0) {
			set_new_color(a);
		}
		if (i % freq_link == 0) {
			create_links(a, generate_link_ratio());
		}
		i = (i + 1) % (freq_link);
		change = propagate(a); 
		catch_up(a); // purely estaetic
		refresh();
		usleep(20000);
	} 
	getchar(); 
	endwin();
}

int main(int arg, char *argv[]) {
	run();
	//test();
	return 0;
}
