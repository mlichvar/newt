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
    newtComponent answer;
    void ** result, **ptr;
    int numselected, i, j;
    
    newtInit();
    newtCls();

    checktree = newtCheckboxTree(-1, -1, 10, NEWT_FLAG_SCROLL);
    newtCheckboxTreeAppend(checktree, 0, "Foobar!", (void *) 1);
    newtCheckboxTreeAppend(checktree, 1, "Baz!", (void *) 2);
    newtCheckboxTreeAppend(checktree, 0, "Really really long thing",
			   (void *) 3);
    newtCheckboxTreeAppend(checktree, 0, "number 4", (void *) 4);
    newtCheckboxTreeAppend(checktree, 1, "number 5", (void *) 5);
    newtCheckboxTreeAppend(checktree, 0, "number 6", (void *) 6);
    newtCheckboxTreeAppend(checktree, 1, "number 7", (void *) 7);
    newtCheckboxTreeAppend(checktree, 0, "number 8", (void *) 8);
    newtCheckboxTreeAppend(checktree, 0, "number 9", (void *) 9);
    newtCheckboxTreeAppend(checktree, 1, "number 10", (void *) 10);
    newtCheckboxTreeAppend(checktree, 0, "number 11", (void *) 11);
    newtCheckboxTreeAppend(checktree, 1, "number 12", (void *) 12);
 
    button = newtButton(-1, -1, "Exit");
    
    grid = newtCreateGrid(1, 2);
    newtGridSetField(grid, 0, 0, NEWT_GRID_COMPONENT, checktree, 0, 0, 0, 0, 
		     NEWT_ANCHOR_RIGHT, 0);
    newtGridSetField(grid, 0, 0, NEWT_GRID_COMPONENT, checktree, 0, 0, 0, 0, 
		     0, 0);
    newtGridSetField(grid, 0, 1, NEWT_GRID_COMPONENT, button, 0, 0, 0, 0, 
		     0, 0);

    newtGridWrappedWindow(grid, "Checkbox Tree Test");
    newtGridFree(grid, 1);

    form = newtForm(NULL, NULL, 0);
    newtFormAddComponents(form, checktree, button, NULL);

    answer = newtRunForm(form);

    newtFinished();

    result = newtCheckboxTreeGetSelection(checktree, &numselected);
    ptr = result;
    if (!result || !numselected)
	printf("none selected\n");
    else
	printf("Current selection:\n");
    for (i = 0; i < numselected; i++) {
	j = (int) *ptr++;
	printf("%d\n", j);
    }
    if (result)
	free(result);
    
    newtFormDestroy(form);
    
    return 0;
}
