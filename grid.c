#include <alloca.h>
#include <stdlib.h>
#include <string.h>

#include "newt.h"
#include "newt_pr.h"

struct gridField {
    enum newtGridElement type;
    union {
	newtGrid grid;
	newtComponent co;
    } u;
    int padLeft, padTop, padRight, padBottom;
    int anchor;
    int flags;
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
		      enum newtGridElement type, void * val, int padLeft,
		      int padTop, int padRight, int padBottom, int anchor,
		      int flags) {
    struct gridField * field = &grid->fields[col][row];

    if (field->type == NEWT_GRID_SUBGRID) 
	newtGridFree(field->u.grid, 1);

    field->type = type;
    field->u.co = (void *) val;

    field->padLeft = padLeft;
    field->padRight = padRight;
    field->padTop = padTop;
    field->padBottom = padBottom;
    field->anchor = anchor;

    grid->width = grid->height = -1;
}

static void shuffleGrid(newtGrid grid, int left, int top, int set) {
    struct gridField * field;
    int row, col;
    int i, j;
    int minWidth, minHeight;
    int * widths, * heights;
    int thisLeft, thisTop;
    int x, y, remx, remy;

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

	    j += field->padLeft + field->padRight;

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

	    j += field->padTop + field->padBottom;

	    i += j;
	    if (j > heights[col]) heights[col] = j;
	}

	if (i > minHeight) minHeight = i;
    }

    /* this catches the -1 case */
    if (grid->width < minWidth) grid->width = minWidth;		/* ack! */
    if (grid->height < minHeight) grid->height = minHeight;	/* ditto! */

    if (!set) return;

    thisTop = top;
    for (row = 0; row < grid->rows; row++) {
	i = 0;
	thisLeft = left;
	for (col = 0; col < grid->cols; col++) {
	    field = &grid->fields[col][row];

	    x = thisLeft + field->padLeft;
	    remx = widths[col] - field->padLeft - field->padRight;
	    y = thisTop + field->padTop;
	    remy = heights[col] - field->padTop - field->padBottom;

	    if (field->type == NEWT_GRID_SUBGRID) {
		remx -= field->u.grid->width;
		remy -= field->u.grid->height;
	    } else {
		remx -= field->u.co->width;
		remy -= field->u.co->height;
	    }

	    if (field->anchor & NEWT_ANCHOR_RIGHT)
		x += remx;
	    else if (!(field->anchor & NEWT_ANCHOR_LEFT))
		x += (remx / 2);
	 
	    if (field->anchor & NEWT_ANCHOR_BOTTOM)
		y += remx;
	    else if (!(field->anchor & NEWT_ANCHOR_TOP))
		y += (remy / 2);
	 

	    if (field->type == NEWT_GRID_SUBGRID) {
		shuffleGrid(field->u.grid, x, y, 1);
	    } else {
		field->u.co->left = x;
		field->u.co->top = y;
		if (field->u.co->ops->place)
		    field->u.co->ops->place(field->u.co);
	    }

	    thisLeft += widths[col];
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

