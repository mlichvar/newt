#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "newt.h"

void main(void) {
    newtComponent b1, b2, c1, c2, r1, r2, r3, e1, e2, e3;
    newtComponent f, chklist;
    char result, result2;
    char * enr1, * enr2, * enr3;

    newtInit();
    newtCls();

    newtOpenWindow(2, 5, 30, 10, "first window");
    newtOpenWindow(20, 10, 40, 13, "window 2");

    f = newtForm();
    chklist = newtForm();

    b1 = newtButton(3, 1, "Push me");
    b2 = newtButton(18, 1, "Not me");
    c1 = newtCheckbox(3, 6, "Check here", ' ', NULL, &result);
    c2 = newtCheckbox(3, 7, "Not here  ", '*', NULL, &result2);
    r1 = newtRadiobutton(20, 6, "Choice 1", 0, NULL);
    r2 = newtRadiobutton(20, 7, "Choice 2", 1, r1);
    r3 = newtRadiobutton(20, 8, "Choice 3", 0, r2);

    e1 = newtEntry(3, 10, "", 20, &enr1, 0);
    e2 = newtEntry(3, 11, "Default", 20, &enr2, NEWT_ENTRY_SCROLL);
    e3 = newtEntry(3, 12, NULL, 20, &enr3, NEWT_ENTRY_HIDDEN);

    newtFormAddComponents(chklist, c1, c2, NULL);

    newtFormAddComponents(f, b1, b2, chklist, NULL);
    newtFormAddComponents(f, r1, r2, r3, e1, e2, e3, NULL);

    newtRunForm(f);
 
    enr1 = strdup(enr1);
    enr2 = strdup(enr2);
    enr3 = strdup(enr3);

    newtFormDestroy(f);

    newtPopWindow();
    newtPopWindow();
    newtFinished();

    printf("got string 1: %s\n", enr1);
    printf("got string 2: %s\n", enr2);
    printf("got string 3: %s\n", enr3);
}
