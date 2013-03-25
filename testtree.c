#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "newt.h"

int main(void) {
    newtGrid grid;
    newtComponent checktree;
    newtComponent button;
    newtComponent form;
    const void **result, **ptr;
    int numselected, i, j;
    int * list;
    
    newtInit();
    newtCls();

    checktree = newtCheckboxTreeMulti(-1, -1, 10, " ab", NEWT_FLAG_SCROLL);
    newtCheckboxTreeAddItem(checktree, "Numbers", (void *) 2, 0,
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "Really really long thing",
			   (void *) 3, 0, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "number 5", (void *) 5, 
    			    NEWT_FLAG_SELECTED, 
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "number 6", (void *) 6, 0, 
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "number 7", (void *) 7, 
    			    NEWT_FLAG_SELECTED, 
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "number 8", (void *) 8, 0, 
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "number 9", (void *) 9, 0, 
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "number 10", (void *) 10,
    			    NEWT_FLAG_SELECTED,
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "number 11", (void *) 11, 0, 
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "Donuts", (void *) 12,
    			    NEWT_FLAG_SELECTED,
    			    NEWT_ARG_APPEND, NEWT_ARG_LAST);

    newtCheckboxTreeAddItem(checktree, "Bavarian Cream", (void *) 301,
    			    NEWT_FLAG_SELECTED,
    			    9, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "Honey dipped", (void *) 302,
    			    NEWT_FLAG_SELECTED,
    			    9, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "Jelly", (void *) 303,
    			    NEWT_FLAG_SELECTED,
    			    9, NEWT_ARG_APPEND, NEWT_ARG_LAST);

    newtCheckboxTreeAddItem(checktree, "Colors", (void *) 1, 0,
    			    0, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "Red", (void *) 100, 0,
    			    0, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "White", (void *) 101, 0,
    			    0, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "Blue", (void *) 102, 0,
    			    0, NEWT_ARG_APPEND, NEWT_ARG_LAST);

    newtCheckboxTreeAddItem(checktree, "number 4", (void *) 4, 0,
    			    3, NEWT_ARG_LAST);

    newtCheckboxTreeAddItem(checktree, "Single digit", (void *) 200, 0,
    			    1, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "One", (void *) 201, 0,
    			    1, 0, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "Two", (void *) 202, 0,
    			    1, 0, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "Three", (void *) 203, 0,
    			    1, 0, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "Four", (void *) 204, 0,
    			    1, 0, NEWT_ARG_APPEND, NEWT_ARG_LAST);

    newtCheckboxTreeAddItem(checktree, "Double digit", (void *) 300, 0,
    			    1, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "Ten", (void *) 210, 0,
    			    1, 1, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "Eleven", (void *) 211, 0,
    			    1, 1, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "Twelve", (void *) 212, 0,
    			    1, 1, NEWT_ARG_APPEND, NEWT_ARG_LAST);
    newtCheckboxTreeAddItem(checktree, "Thirteen", (void *) 213, 0,
    			    1, 1, NEWT_ARG_APPEND, NEWT_ARG_LAST);

    newtCheckboxTreeSetCurrent(checktree, (void *) 12);

    button = newtButton(-1, -1, "Exit");
    
    grid = newtCreateGrid(1, 2);
    newtGridSetField(grid, 0, 0, NEWT_GRID_COMPONENT, checktree, 0, 0, 0, 1, 
		     NEWT_ANCHOR_RIGHT, 0);
    newtGridSetField(grid, 0, 1, NEWT_GRID_COMPONENT, button, 0, 0, 0, 0, 
		     0, 0);

    newtGridWrappedWindow(grid, "Checkbox Tree Test");
    newtGridFree(grid, 1);

    form = newtForm(NULL, NULL, 0);
    newtFormAddComponents(form, checktree, button, NULL);

    newtRunForm(form);

    newtFinished();

    result = newtCheckboxTreeGetSelection(checktree, &numselected);
    ptr = result;
    if (!result || !numselected)
	printf("none selected\n");
    else
	printf("Current selection (all) (%d):\n", numselected);
    for (i = 0; i < numselected; i++) {
	j = (long) *ptr++;
	printf("%d\n", j);
    }
    free(result);

    result = newtCheckboxTreeGetMultiSelection(checktree, &numselected, 'b');
    ptr = result;
    if (!result || !numselected)
	printf("none selected\n");
    else
	printf("Current selection (b) (%d):\n",numselected);
    for (i = 0; i < numselected; i++) {
	j = (long) *ptr++;
	printf("%d\n", j);
    }
	
    if (result)
	free(result);

    list = newtCheckboxTreeFindItem(checktree, (void *) 213);
    printf("path:");
    for (i = 0; list && list[i] != NEWT_ARG_LAST; i++)
        printf(" %d", list[i]);
    printf("\n");
    free(list);
    
    newtFormDestroy(form);
    
    return 0;
}
