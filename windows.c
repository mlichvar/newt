#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "errno.h"
#include "newt.h"

static void * newtvwindow(char * title, char * button1, char * button2, 
		       char * button3, char * message, va_list args) {
    newtComponent b1, b2 = NULL, b3 = NULL, t, f, answer;
    char * buf = NULL;
    int size = 0;
    int i = 0;
    int width, height;
    char * flowedText;
    newtGrid grid, buttonGrid;

    do {
	size += 1000;
	if (buf) free(buf);
	buf = malloc(size);
	i = vsnprintf(buf, size, message, args);
    } while (i == size);

    flowedText = newtReflowText(buf, 35, 5, 5, &width, &height);
    if (height > 6) {
	free(flowedText);
	flowedText = newtReflowText(buf, 60, 5, 5, &width, &height);
    }
    free(buf);

    t = newtTextbox(-1, -1, width, height, NEWT_TEXTBOX_WRAP);
    newtTextboxSetText(t, flowedText);
    free(flowedText);

    if (button3) {
	buttonGrid = newtButtonBar(button1, &b1, button2, &b2, 
				   button3, &b3, NULL);
    } else if (button2) {
	buttonGrid = newtButtonBar(button1, &b1, button2, &b2, NULL);
    } else {
	buttonGrid = newtButtonBar(button1, &b1, NULL);
    }

    newtGridSetField(buttonGrid, 0, 0, NEWT_GRID_COMPONENT, b1, 
		     0, 0, button2 ? 1 : 0, 0, 0, 0);

    grid = newtGridVStacked(NEWT_GRID_COMPONENT, t,
			    NEWT_GRID_SUBGRID, buttonGrid,
			    NEWT_GRID_EMPTY);
    newtGridWrappedWindow(grid, title);

    f = newtForm(NULL, NULL, 0);
    newtFormAddComponents(f, t, b1, NULL);

    if (button2)
	newtFormAddComponent(f, b2);
    if (button3)
	newtFormAddComponent(f, b3);

    answer = newtRunForm(f);
    newtGridFree(grid, 1);
 
    newtFormDestroy(f);
    newtPopWindow();

    if (answer == f)
	return NULL;
    else if (answer == b1)
	return button1;
    else if (answer == b2)
	return button2;

    return button3;
}

int newtWinChoice(char * title, char * button1, char * button2, 
		   char * message, ...) {
    va_list args;
    void * rc;

    va_start(args, message);
    rc = newtvwindow(title, button1, button2, NULL, message, args);
    va_end(args);

    if (rc == button1)
	return 1;
    else if (rc == button2)
	return 2;

    return 0;
}

void newtWinMessage(char * title, char * buttonText, char * text, ...) {
    va_list args;

    va_start(args, text);
    newtvwindow(title, buttonText, NULL, NULL, text, args);
    va_end(args);
}

void newtWinMessagev(char * title, char * buttonText, char * text, 
		     va_list argv) {
    newtvwindow(title, buttonText, NULL, NULL, text, argv);
}

int newtWinTernary(char * title, char * button1, char * button2, 
		   char * button3, char * message, ...) {
    va_list args;
    void * rc;

    va_start(args, message);
    rc = newtvwindow(title, button1, button2, button3, message, args);
    va_end(args);

    if (rc == button1)
	return 1;
    else if (rc == button2)
	return 2;
    else if (rc == button3)
	return 3;

    return 0;
}

/* only supports up to 50 buttons -- shucks! */
int newtWinMenu(char * title, char * text, int suggestedWidth, int flexDown, 
		int flexUp, int maxListHeight, char ** items, int * listItem,
		char * button1, ...) {
    char * reflowedText;
    int width, height;
    newtComponent textbox, listbox, result, form;
    va_list args;
    newtComponent buttons[50];
    newtGrid grid, buttonBar;
    int numButtons;
    int i, rc;
    int needScroll;
    char * buttonName;

    reflowedText = newtReflowText(text, suggestedWidth, flexDown, flexUp,
				  &width, &height);
    
    textbox = newtTextbox(-1, -1, width, height, NEWT_TEXTBOX_WRAP);
    newtTextboxSetText(textbox, reflowedText);

    for (i = 0; items[i]; i++) ;
    if (i < maxListHeight) maxListHeight = i;
    needScroll = i > maxListHeight;

    listbox = newtListbox(-1, -1, maxListHeight, 
		  (needScroll ? 0 : NEWT_FLAG_NOSCROLL) | NEWT_FLAG_RETURNEXIT);
    for (i = 0; items[i]; i++) {
	newtListboxAddEntry(listbox, items[i], (void *) i);
    }

    newtListboxSetCurrent(listbox, *listItem);

    buttonName = button1, numButtons = 0;
    va_start(args, button1);
    while (buttonName) {
	buttons[numButtons] = newtButton(-1, -1, buttonName);
	numButtons++;
	buttonName = va_arg(args, char *);
    }

    va_end(button1);

    buttonBar = newtCreateGrid(numButtons, 1);
    for (i = 0; i < numButtons; i++) {
	newtGridSetField(buttonBar, i, 0, NEWT_GRID_COMPONENT, 
			 buttons[i],
			 i ? 1 : 0, 0, 0, 0, 0, 0);
    }

    grid = newtGridVStacked(NEWT_GRID_COMPONENT, textbox,
			    NEWT_GRID_COMPONENT, listbox,
			    NEWT_GRID_SUBGRID, buttonBar,
			    NEWT_GRID_EMPTY);

    newtGridWrappedWindow(grid, title);

    form = newtForm(NULL, 0, 0);
    newtGridAddComponentsToForm(grid, form, 1);
    newtGridFree(grid, 1);

    result = newtRunForm(form);

    *listItem = ((long) newtListboxGetCurrent(listbox));

    for (rc = 0; result != buttons[rc] && rc < numButtons; rc++);
    if (rc == numButtons) 
	rc = 0; /* F12 or return-on-exit (which are the same for us) */
    else 
	rc++;

    newtFormDestroy(form);
    newtPopWindow();

    return rc;
}
