#include <alloca.h>
#include <stdlib.h>
#include <string.h>

#include "grid.h"
#include "newt.h"
#include "newt_pr.h"

struct gridField {
    enum newtGridElement type;
    union {
	newtGrid grid;
	newtComponent co;
    } u;
}; 

struct grid_s {
    int rows, cols;
    int width, height;		/* totals, -1 means unknown */
    struct gridField ** fields;
};

newtGrid newtCreateGrid(int cols, int rows) {
    newtGrid grid;

    grid = malloc(sizeof(*grid));
    grid->rows = rows;
    grid->cols = cols;

    grid->fields = malloc(sizeof(*grid->fields) * cols);
    while (cols--) {
	grid->fields[cols] = malloc(sizeof(**(grid->fields)) * rows);
	memset(grid->fields[cols], 0, sizeof(**(grid->fields)) * rows);
    }

    grid->width = grid->height = -1;

    return grid;
}

void newtGridSetField(newtGrid grid, int col, int row, 
		      enum newtGridElement type, void * val) {
    struct gridField * field = &grid->fields[col][row];

    if (field->type == NEWT_GRID_SUBGRID) 
	newtGridFree(field->u.grid, 1);

    field->type = type;
    field->u.co = (void *) val;

    grid->width = grid->height = -1;
}

static void shuffleGrid(newtGrid grid, int left, int top, int set) {
    struct gridField * field;
    int row, col;
    int i, j;
    int minWidth, minHeight;
    int colSpacing;
    int * widths, * heights;
    int thisLeft, thisTop;

    widths = alloca(sizeof(*widths) * grid->cols);
    memset(widths, 0, sizeof(*widths) * grid->cols);
    heights = alloca(sizeof(*heights) * grid->rows);
    memset(heights, 0, sizeof(*heights) * grid->rows);

    minWidth = 0;
    for (row = 0; row < grid->rows; row++) {
	i = 0;
	for (col = 0; col < grid->cols; col++) {
	    field = &grid->fields[col][row];
	    if (field->type == NEWT_GRID_SUBGRID) {
		/* we'll have to redo this later */
		if (field->u.grid->width == -1) 
		    shuffleGrid(field->u.grid, left, top, 0);
		j = field->u.grid->width;
	    } else {
		j = field->u.co->width;
	    }

	    i += j;
	    if (j > widths[col]) widths[col] = j;
	}

	if (i > minWidth) minWidth = i;
    }

    minHeight = 0;
    for (col = 0; col < grid->cols; col++) {
	i = 0;
	for (row = 0; row < grid->rows; row++) {
	    field = &grid->fields[col][row];
	    if (field->type == NEWT_GRID_SUBGRID) {
		/* we'll have to redo this later */
		if (field->u.grid->height == -1) 
		    shuffleGrid(field->u.grid, left, top, 0);
		j = field->u.grid->height;
	    } else {
		j = field->u.co->height;
	    }

	    i += j;
	    if (j > heights[col]) heights[col] = j;
	}

	if (i > minHeight) minHeight = i;
    }

    if (grid->width == -1) {
	colSpacing = 1;
    } else {
	colSpacing = (grid->width - minWidth) / (grid->cols - 1);
    }
    minWidth += colSpacing * (grid->cols - 1);

    if (minWidth < grid->width) grid->width = minWidth;		/* ack! */
    if (minHeight < grid->height) grid->height = minHeight;	/* ditto! */

    if (!set) return;

    thisTop = top;
    for (row = 0; row < grid->rows; row++) {
	i = 0;
	thisLeft = left;
	for (col = 0; col < grid->cols; col++) {
	    field = &grid->fields[col][row];
	    if (field->type == NEWT_GRID_SUBGRID) {
		shuffleGrid(field->u.grid, thisLeft, thisTop, 1);
	    } else {
		field->u.co->top = thisTop;
		field->u.co->left = thisLeft;
		if (field->u.co->ops->place)
		    field->u.co->ops->place(field->u.co);
	    }

	    thisLeft += widths[col] + colSpacing;
	}

	thisTop += heights[row];
    }
}

void newtGridPlace(newtGrid grid, int left, int top) {
    grid->width = grid->height = -1;

    shuffleGrid(grid, left, top, 1);
}

void newtGridFree(newtGrid grid, int recurse) {
    int row, col;

    for (col = 0; col < grid->cols; col++) {
	if (recurse) {
	    for (row = 0; row < grid->rows; row++) {
		if (grid->fields[col][row].type == NEWT_GRID_SUBGRID)
		    newtGridFree(grid->fields[col][row].u.grid, 1);
	    }
	}

	free(grid->fields[col]);
    }

    free(grid->fields);
    free(grid);
}

