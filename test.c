#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include "newt.h"

struct callbackInfo {
    newtComponent en;
    char * state;
};

void disableCallback(newtComponent co, void * data) {
    struct callbackInfo * cbi = data;

    if (*cbi->state == ' ') {
	newtEntrySetFlags(cbi->en, NEWT_FLAG_DISABLED, NEWT_FLAGS_RESET);
    } else {
	newtEntrySetFlags(cbi->en, NEWT_FLAG_DISABLED, NEWT_FLAGS_SET);
    }

    newtRefresh();
}

void suspend(void) {
    newtSuspend();
    raise(SIGTSTP);
    newtResume();
}

void main(void) {
    newtComponent b1, b2, r1, r2, r3, e2, e3, l1, l2, l3, scale;
    newtComponent lb, t, rsf, answer;
    newtComponent cs[10];
    newtComponent f, chklist, e1;
    struct callbackInfo cbis[3];
    char results[10];
    char * enr2, * enr3, * scaleVal;
    void ** selectedList;
    int i;
    char buf[20];

    newtInit();
    newtCls();

    newtSetSuspendCallback(suspend);

    newtDrawRootText(0, 0, "Newt test program");
    newtPushHelpLine(NULL);

    newtOpenWindow(2, 2, 30, 10, "first window");
    newtOpenWindow(10, 5, 65, 16, "window 2");

    f = newtForm(NULL, NULL, 0);
    chklist = newtForm(NULL, NULL, 0);

    b1 = newtButton(3, 1, "Exit");
    b2 = newtButton(18, 1, "Update");
    r1 = newtRadiobutton(20, 10, "Choice 1", 0, NULL);
    r2 = newtRadiobutton(20, 11, "Chc 2", 1, r1);
    r3 = newtRadiobutton(20, 12, "Choice 3", 0, r2);
    rsf = newtForm(NULL, NULL, 0);
    newtFormAddComponents(rsf, r1, r2, r3, NULL);
    newtFormSetBackground(rsf, NEWT_COLORSET_CHECKBOX);

    for (i = 0; i < 10; i++) {
	sprintf(buf, "Check %d", i);
	cs[i] = newtCheckbox(3, 10 + i, buf, ' ', NULL, &results[i]);
	newtFormAddComponent(chklist, cs[i]);
    }

    l1 = newtLabel(3, 6, "Scale:");
    l2 = newtLabel(3, 7, "Scrolls:");
    l3 = newtLabel(3, 8, "Hidden:");
    e1 = newtEntry(12, 6, "", 20, &scaleVal, 0);
    e2 = newtEntry(12, 7, "Default", 20, &enr2, NEWT_FLAG_SCROLL);
    e3 = newtEntry(12, 8, NULL, 20, &enr3, NEWT_FLAG_HIDDEN);

    cbis[0].state = &results[0];
    cbis[0].en = e1;
    newtComponentAddCallback(cs[0], disableCallback, &cbis[0]);

    scale = newtScale(3, 14, 32, 100);

    newtFormSetHeight(chklist, 3);

    newtFormAddComponents(f, b1, b2, l1, l2, l3, e1, e2, e3, chklist, NULL);
    newtFormAddComponents(f, rsf, scale, NULL);

    lb = newtListbox(45, 3, 4, NEWT_FLAG_MULTIPLE);
    newtListboxAddEntry(lb, "First", "First");
    newtListboxAddEntry(lb, "Second", "Second");
    newtListboxAddEntry(lb, "Third", "Third");
    newtListboxAddEntry(lb, "Fourth", "Fourth");
    newtListboxAddEntry(lb, "Fifth", "Fifth");
    newtListboxAddEntry(lb, "Sixth", "Sixth");
    newtListboxAddEntry(lb, "Seventh", "Seventh");
    newtListboxAddEntry(lb, "Eighth", "Eighth");
    newtListboxAddEntry(lb, "Ninth", "Ninth");
    newtListboxAddEntry(lb, "Tenth", "Tenth");

    t = newtTextbox(45, 10, 17, 5, NEWT_FLAG_WRAP);
    newtTextboxSetText(t, "This is some text does it look okay?\nThis should be alone.\nThis shouldn't be printed");

    newtFormAddComponents(f, lb, t, NULL);
    newtRefresh();

    do {
	answer = newtRunForm(f);
	
	if (answer == b2) {
	    newtScaleSet(scale, atoi(scaleVal));
	    newtRefresh();
	    answer = NULL;
	}
    } while (!answer);
 
    scaleVal = strdup(scaleVal);
    enr2 = strdup(enr2);
    enr3 = strdup(enr3);

    selectedList = newtListboxGetSelection(lb);

    newtFormDestroy(f);

    newtPopWindow();
    newtPopWindow();
    newtFinished();

    printf("got string 1: %s\n", scaleVal);
    printf("got string 2: %s\n", enr2);
    printf("got string 3: %s\n", enr3);

    if(selectedList) {
	printf("\nSelected listbox items:\n");
	for(i = 0; selectedList[i]; i++)
	    puts(selectedList[i]);
    }
}
