#include "newt.h"
#include <stdarg.h>

newtGrid newtButtonBar(char * button1, newtComponent * b1comp, ...) {
    va_list args;
    char * name;
    newtGrid grid;
    newtComponent * compPtr;
    int num;

    va_start(args, b1comp);

    name = button1, num = 0;
    while (name) {
	name = va_arg(args, char *);
	compPtr = va_arg(args, newtComponent *);
	num++;
    }

    va_end(args);

    grid = newtCreateGrid(num, 1);

    va_start(args, b1comp);

    name = button1, compPtr = b1comp, num = 0;
    while (name) {
	*compPtr = newtButton(-1, -1, name);
	newtGridSetField(grid, num, 0, NEWT_GRID_COMPONENT, *compPtr, 
			 num ? 1 : 0, 0, 0, 0, 0, 0);

	name = va_arg(args, char *);
	compPtr = va_arg(args, newtComponent *);
	num++;
    }

    return grid;
}
