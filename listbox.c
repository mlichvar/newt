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
    int curr;
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
    newtComponent co;
    struct listbox * li;

    co = malloc(sizeof(*co));
    li = malloc(sizeof(struct listbox));

    li->allocedItems = 5;
    li->numItems = 0; 
    li->flags = 0;
    li->items = malloc(li->allocedItems * sizeof(*li->items));
    li->form = newtForm();

    if (height)
	newtFormSetHeight(li->form, height);

    co->data = li;
    co->left = left;
    co->top = top;
    co->height = li->form->height;
    co->width = li->form->width;
    co->ops = &listboxOps;
    co->takesFocus = 1;

    return co;
}

void newtListboxAddEntry(newtComponent co, char * text) {
    struct listbox * li = co->data;

    if (li->numItems == li->allocedItems) {
	li->allocedItems += 5;
	li->items = realloc(li->items, li->allocedItems * sizeof(*li->items));
    }

    if (li->numItems)
	li->items[li->numItems] = newtListitem(co->left, 
					       li->numItems + co->top, 
					       text, 0,
					       li->items[li->numItems - 1]);
    else
	li->items[li->numItems] = newtListitem(co->left, 
					       li->numItems + co->top, 
					       text, 0, NULL);

    /*newtLabelColor(li->items[li->numItems], COLORSET_ACTLISTBOX,
			COLORSET_LISTBOX);*/

    newtFormAddComponent(li->form, li->items[li->numItems]);
    li->numItems++;

    co->height = li->form->height;
    co->width = li->form->width;
}

static void listboxDraw(newtComponent co) {
    struct listbox * li = co->data;

    li->form->ops->draw(li->form);
}

static struct eventResult listboxEvent(newtComponent co, struct event ev) {
    struct listbox * li = co->data;

    return li->form->ops->event(li->form, ev);
}

static void listboxDestroy(newtComponent co) {
    struct listbox * li = co->data;

    li->form->ops->destroy(li->form);
    free(li->items);
    free(li);
    free(co);
}
