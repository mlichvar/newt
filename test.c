#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "newt.h"

void main(void) {
    newtComponent b1, b2, r1, r2, r3, e1, e2, e3, l1, l2, l3;
    newtComponent lb, t;
    newtComponent cs[10];
    newtComponent f, chklist;
    char results[10];
    char * enr1, * enr2, * enr3;
    int i;
    char buf[20];

    newtInit();
    newtCls();

    newtOpenWindow(2, 2, 30, 10, "first window");
    newtOpenWindow(10, 5, 65, 16, "window 2");

    f = newtForm(NULL);
    chklist = newtForm(NULL);

    b1 = newtButton(3, 1, "Push me");
    b2 = newtButton(18, 1, "Not me");
    r1 = newtRadiobutton(20, 6, "Choice 1", 0, NULL);
    r2 = newtRadiobutton(20, 7, "Choice 2", 1, r1);
    r3 = newtRadiobutton(20, 8, "Choice 3", 0, r2);

    for (i = 0; i < 10; i++) {
	sprintf(buf, "Check %d", i);
	cs[i] = newtCheckbox(3, 6 + i, buf, ' ', NULL, &results[i]);
	newtFormAddComponent(chklist, cs[i]);
    }

    l1 = newtLabel(3, 10, "Fixed:");
    l2 = newtLabel(3, 11, "Scrolls:");
    l3 = newtLabel(3, 12, "Hidden:");
    e1 = newtEntry(12, 10, "", 20, &enr1, 0);
    e2 = newtEntry(12, 11, "Default", 20, &enr2, NEWT_ENTRY_SCROLL);
    e3 = newtEntry(12, 12, NULL, 20, &enr3, NEWT_ENTRY_HIDDEN);

    newtFormSetHeight(chklist, 3);

    newtFormAddComponents(f, b1, b2, chklist, NULL);
    newtFormAddComponents(f, r1, r2, r3, l1, l2, l3, e1, e2, e3, NULL);

    lb = newtListbox(45, 3, 4, 0);
    newtListboxAddEntry(lb, "First", NULL);
    newtListboxAddEntry(lb, "Second", NULL);
    newtListboxAddEntry(lb, "Third", NULL);
    newtListboxAddEntry(lb, "Fourth", NULL);
    newtListboxAddEntry(lb, "Fifth", NULL);
    newtListboxAddEntry(lb, "Sixth", NULL);
    newtListboxAddEntry(lb, "Seventh", NULL);
    newtListboxAddEntry(lb, "Eighth", NULL);
    newtListboxAddEntry(lb, "Ninth", NULL);
    newtListboxAddEntry(lb, "Tenth", NULL);

    t = newtTextbox(45, 10, 17, 5, NEWT_TEXTBOX_WRAP);
    newtTextboxSetText(t, "This is some text does it look okay?\nThis should be alone.\nThis shouldn't be printed");

    newtFormAddComponents(f, lb, t, NULL);

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
