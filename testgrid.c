#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "newt.h"

int main(void) {
    newtComponent b1, b2, b3, b4;
    newtComponent answer, f, t;
    newtGrid grid, subgrid;

    newtInit();
    newtCls();
    newtOpenWindow(2, 2, 40, 15, "first window");

    b1 = newtButton(-1, -1, "Button 1");
    b2 = newtButton(-1, -1, "Another Button");
    b3 = newtButton(-1, -1, "But, but");
    b4 = newtButton(-1, -1, "But what?");

    f = newtForm(NULL, NULL, 0);

    grid = newtCreateGrid(2, 2);
    newtGridSetField(grid, 0, 0, NEWT_GRID_COMPONENT, b1, 0, 0, 0, 0, 
			NEWT_ANCHOR_RIGHT, 0);
    newtGridSetField(grid, 0, 1, NEWT_GRID_COMPONENT, b2, 0, 0, 0, 0, 0, 0);
    newtGridSetField(grid, 1, 0, NEWT_GRID_COMPONENT, b3, 0, 0, 0, 0, 0, 0);
    newtGridSetField(grid, 1, 1, NEWT_GRID_COMPONENT, b4, 0, 0, 0, 0, 0, 0);

    newtGridPlace(grid, 1, 1);

    newtFormAddComponents(f, b1, b2, b3, b4, NULL);

    answer = newtRunForm(f);
	
    newtFormDestroy(f);

    newtPopWindow();

    t = newtTextbox(-1, -1, 40, 4, NEWT_FLAG_WRAP);
    newtTextboxSetText(t, "This is a quite a bit of text. It is 40 "
			  "columns long, so some wrapping should be "
			  "done. Did you know that the quick, brown "
			  "fox jumped over the lazy dog?");
    
    b1 = newtButton(-1, -1, "Okay");
    b2 = newtButton(-1, -1, "Cancel");

    grid = newtCreateGrid(1, 2);
    subgrid = newtCreateGrid(2, 1);

    newtGridSetField(subgrid, 0, 0, NEWT_GRID_COMPONENT, b1, 0, 0, 0, 0, 0, 0);
    newtGridSetField(subgrid, 1, 0, NEWT_GRID_COMPONENT, b2, 0, 0, 0, 0, 0, 0);

    newtGridSetField(grid, 0, 0, NEWT_GRID_COMPONENT, t, 0, 0, 0, 1, 0, 0);
    newtGridSetField(grid, 0, 1, NEWT_GRID_SUBGRID, subgrid, 0, 0, 0, 0, 0,
			NEWT_GRID_FLAG_GROWX);
    newtGridWrappedWindow(grid, "another example");

    f = newtForm(NULL, NULL, 0);
    newtFormAddComponents(f, b1, t, b2, NULL);
    answer = newtRunForm(f);

    newtFinished();

    return 0;
}
