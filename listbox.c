#include <slang/slang.h>
#include <stdlib.h>
#include <string.h>

#include "newt.h"
#include "newt_pr.h"

struct listbox {
    newtComponent * items, form;
    int numItems;
    int allocedItems;
    int flags;
    newtComponent sb;
};

static void listboxDraw(newtComponent co);
static void listboxDestroy(newtComponent co);
static struct eventResult listboxEvent(newtComponent co, struct event ev);

static struct componentOps listboxOps = {
    listboxDraw,
    listboxEvent,
    listboxDestroy,
} ;

newtComponent newtListbox(int left, int top, int height, int flags) {
    newtComponent co, sb;
    struct listbox * li;

    co = malloc(sizeof(*co));
    li = malloc(sizeof(struct listbox));

    li->allocedItems = 5;
    li->numItems = 0; 
    li->flags = flags;
    li->items = malloc(li->allocedItems * sizeof(*li->items));

    if (height) 
	sb = newtVerticalScrollbar(left, top, height, COLORSET_LISTBOX,
				   COLORSET_ACTLISTBOX);
    else
	sb = NULL;
    li->form = newtForm(sb, NULL, NEWT_FORM_NOF12);
    li->sb = sb;

    if (height) {
	newtFormSetHeight(li->form, height);
	newtFormAddComponent(li->form, sb);
    }

    co->data = li;
    co->left = left;
    co->top = top;
    co->height = li->form->height;
    co->width = li->form->width;
    co->ops = &listboxOps;
    co->takesFocus = 1;

    return co;
}

void newtListboxSetCurrent(newtComponent co, int num) {
    struct listbox * li = co->data;

    newtFormSetCurrent(li->form, li->items[num]);
}

void * newtListboxGetCurrent(newtComponent co) {
    struct listbox * li = co->data;
    newtComponent curr;
    int i;

    /* Having to do this linearly is really, really dumb */

    curr = newtFormGetCurrent(li->form);
    for (i = 0; i < li->numItems; i++) {
	if (li->items[i] == curr) break;
    }
 
    if (li->items[i] == curr) 
        return newtListitemGetData(li->items[i]);
    else
	return NULL;
}

void newtListboxSetEntry(newtComponent co, int num, char * text) {
    struct listbox * li = co->data;

    /* this won't increase the size of the listbox! */

    newtListitemSet(li->items[num], text);
    co->ops->draw(co);
}

void newtListboxAddEntry(newtComponent co, char * text, void * data) {
    struct listbox * li = co->data;

    if (li->numItems == li->allocedItems) {
	li->allocedItems += 5;
	li->items = realloc(li->items, li->allocedItems * sizeof(*li->items));
    }

    if (li->numItems)
	li->items[li->numItems] = newtListitem(co->left, 
					       li->numItems + co->top, 
					       text, 0,
					       li->items[li->numItems - 1],
					       data);
    else
	li->items[li->numItems] = newtListitem(co->left, 
					       li->numItems + co->top, 
					       text, 0, NULL, data);

    newtFormAddComponent(li->form, li->items[li->numItems]);
    li->numItems++;

    co->height = li->form->height;
    co->width = li->form->width;

    if (li->sb)
	li->sb->left = co->left + co->width;
}

static void listboxDraw(newtComponent co) {
    struct listbox * li = co->data;

    li->form->ops->draw(li->form);
}

static struct eventResult listboxEvent(newtComponent co, struct event ev) {
    struct listbox * li = co->data;
    struct eventResult er;

    if ((li->flags & NEWT_LISTBOX_RETURNEXIT) && ev.when == EV_NORMAL && 
	ev.event == EV_KEYPRESS && ev.u.key == '\r') {
	er.result = ER_EXITFORM;
	return er;
    }

    return li->form->ops->event(li->form, ev);
}

static void listboxDestroy(newtComponent co) {
    struct listbox * li = co->data;

    li->form->ops->destroy(li->form);
    free(li->items);
    free(li);
    free(co);
}
