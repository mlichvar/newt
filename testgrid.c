#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "grid.h"
#include "newt.h"

int main(void) {
    newtComponent b1, b2, b3, b4;
    newtComponent answer, f;
    newtGrid grid;

    newtInit();
    newtCls();
    newtOpenWindow(2, 2, 40, 15, "first window");

    b1 = newtButton(-1, -1, "Button 1");
    b2 = newtButton(-1, -1, "Another Button");
    b3 = newtButton(-1, -1, "But, but");
    b4 = newtButton(-1, -1, "But what?");

    f = newtForm(NULL, NULL, 0);

    grid = newtCreateGrid(2, 2);
    newtGridSetField(grid, 0, 0, NEWT_GRID_COMPONENT, b1);
    newtGridSetField(grid, 0, 1, NEWT_GRID_COMPONENT, b2);
    newtGridSetField(grid, 1, 0, NEWT_GRID_COMPONENT, b3);
    newtGridSetField(grid, 1, 1, NEWT_GRID_COMPONENT, b4);

    newtGridPlace(grid, 1, 1);

    newtFormAddComponents(f, b1, b2, b3, b4, NULL);

    answer = newtRunForm(f);
	
    newtFormDestroy(f);

    newtPopWindow();
    newtFinished();

    return 0;
}
