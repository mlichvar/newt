/* This goofed-up box whacked into shape by Elliot Lee <sopwith@cuc.edu>
   (from the original listbox by Erik Troan <ewt@redhat.com>)
   and contributed to newt for use under the LGPL license.
   Copyright (C) 1996, 1997 Elliot Lee */

#include <slang/slang.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "newt.h"
#include "newt_pr.h"


/* Linked list of items in the listbox */
struct items {
    void *key, *data;
    unsigned char isSelected;
    struct items *next;
};

/* Holds all the relevant information for this listbox */
struct listbox {
    newtComponent sb; /* Scrollbar on right side of listbox */
    int numItems, curWidth, numSelected;
    int userHasSetWidth;
    int currItem, startShowItem; /* startShowItem is the first item displayed
				   on the screen */
    int isActive; /* If we handle key events all the time, it seems
		     to do things even when they are supposed to be for
		     another button/whatever */
    struct items *boxItems;
    int grow;
    int flags; /* flags for this listbox, right now just
		  NEWT_FLAG_RETURNEXIT */
};

static void listboxDraw(newtComponent co);
static void listboxDestroy(newtComponent co);
static struct eventResult listboxEvent(newtComponent co, struct event ev);
static void newtListboxRealSetCurrent(newtComponent co);

static struct componentOps listboxOps = {
    listboxDraw,
    listboxEvent,
    listboxDestroy,
};

newtComponent newtListbox(int left, int top, int height, int flags) {
    newtComponent co, sb;
    struct listbox * li;

    if (!(co = malloc(sizeof(*co))))
	return NULL;

    if (!(li = malloc(sizeof(struct listbox)))) {
	free(co);
	return NULL;
    }

    li->boxItems = NULL;
    li->numItems = 0;
    li->currItem = 0;
    li->numSelected = 0;
    li->isActive = 0;
    li->userHasSetWidth = 0;
    li->startShowItem = 0;
    li->flags = flags & (NEWT_FLAG_RETURNEXIT|NEWT_FLAG_DOBORDER|NEWT_FLAG_MULTIPLE);

    if (height) {
	li->grow = 0;
	if (flags & NEWT_FLAG_NOSCROLL)
	    sb = NULL;
	else
	    sb = newtVerticalScrollbar(left, top, height, COLORSET_LISTBOX,
				       COLORSET_ACTLISTBOX);
    } else {
	li->grow = 1;
	sb = NULL;
    }

    li->sb = sb;
    co->data = li;
    co->left = left;
    co->top = top;
    co->height = height;
    li->curWidth = 5;
    co->ops = &listboxOps;
    co->takesFocus = 1;
    co->callback = NULL;

    return co;
}

void newtListboxSetCurrent(newtComponent co, int num)
{
    struct listbox * li = co->data;
    if (num >= li->numItems)
	li->currItem = li->numItems - 1;
    else if (num < 0)
	li->currItem = 0;
    else
	li->currItem = num;

    if (li->currItem < li->startShowItem)
	li->startShowItem = li->currItem;
    else if (li->currItem - li->startShowItem > co->height - 1)
	li->startShowItem = li->currItem - co->height + 1;
    if (li->startShowItem + co->height > li->numItems)
	li->startShowItem = li->numItems - co->height;
    if(li->startShowItem < 0)
	li->startShowItem = 0;
    newtListboxRealSetCurrent(co);
}

static void
newtListboxRealSetCurrent(newtComponent co)
{
    struct listbox * li = co->data;
    if(li->sb)
	newtScrollbarSet(li->sb, li->currItem + 1, li->numItems);
    listboxDraw(co);
    if(co->callback) co->callback(co, co->callbackData);
}

void newtListboxSetWidth(newtComponent co , int width) {
    struct listbox * li = co->data;
    
    li->curWidth = co->width = width;
    li->userHasSetWidth = 1;
    li->sb->left = width + co->left + 2;
    listboxDraw(co);
}

void * newtListboxGetCurrent(newtComponent co) {
    struct listbox * li = co->data;
    int i;
    struct items *item;

    for(i = 0, item = li->boxItems; item != NULL && i < li->currItem;
	i++, item = item->next);

    if (item)
	return item->data;
    else
	return NULL;
}

void newtListboxSelectItem(newtComponent co, int item)
{
    struct listbox * li = co->data;
    int i;
    struct items *iitem;
    
    for(i = 0, iitem = li->boxItems; iitem != NULL && i < item;
	i++, iitem = iitem->next);

    if (iitem) {
	if(iitem->isSelected)
	    li->numSelected--;
	else
	    li->numSelected++;
	iitem->isSelected = !iitem->isSelected;
    }
    listboxDraw(co);
}

void newtListboxClearSelection(newtComponent co)
{
    struct items *item;
    struct listbox * li = co->data;

    for(item = li->boxItems; item != NULL;
	item = item->next)
	item->isSelected = 0;
    
    listboxDraw(co);
}

/* Free the returned array after use, but NOT the values in the array */
void ** newtListboxGetSelection(newtComponent co)
{
    struct listbox * li;
    int i;
    void **retval;
    struct items *item;

    if(!co) return NULL;

    li = co->data;
    if(!li || !li->numSelected) return NULL;

    retval = malloc((li->numSelected + 1) * sizeof(void *));
    for(i = 0, item = li->boxItems; item != NULL;
	item = item->next)
	if(item->isSelected)
	    retval[i++] = item->data;
    retval[i] = NULL;
    return retval;
}

void newtListboxSetText(newtComponent co, int num, char * text) {
    struct listbox * li = co->data;
    int i;
    struct items *item;

    for(i = 0, item = li->boxItems; item != NULL && i < num;
	i++, item = item->next);

    if(!item)
	return;
    else {
	free(item->key);
	item->key = strdup(text);
    }
    if (li->userHasSetWidth == 0
	&& strlen(text) > li->curWidth) {
	co->width = li->curWidth = strlen(text);
	if (li->sb)
	    li->sb->left = co->left + co->width + 2;
    }

    if (num >= li->startShowItem && num <= li->startShowItem + co->height)
	listboxDraw(co);
}

void newtListboxSetEntry(newtComponent co, int num, char * text) {
    newtListboxSetText(co, num, text);
}

void newtListboxSetData(newtComponent co, int num, void * data) {
    struct listbox * li = co->data;
    int i;
    struct items *item;

    for(i = 0, item = li->boxItems; item != NULL && i < num;
	i++, item = item->next);

    item->data = data;
}

int newtListboxAddEntry(newtComponent co, char * text, void * data) {
    struct listbox * li = co->data;
    struct items *item;

    if(li->boxItems) {
	for (item = li->boxItems; item->next != NULL; item = item->next);

	item = item->next = malloc(sizeof(struct items));
    } else {
	item = li->boxItems = malloc(sizeof(struct items));
    }

    if (li->userHasSetWidth == 0
	&& text && (strlen(text) > li->curWidth))
	li->curWidth = strlen(text) ;

    item->key = strdup(text); item->data = data; item->next = NULL;
    item->isSelected = 0;
    
    if (li->sb)
	li->sb->left = co->left + li->curWidth + 2;

    if (li->grow)
	co->height++;
    if(li->userHasSetWidth == 0)
	co->width = li->curWidth;
    li->numItems++;

    listboxDraw(co);
    
    return li->numItems;
}


int newtListboxInsertEntry(newtComponent co, char * text, void * data, 
			   int num) {
    struct listbox * li = co->data;
    struct items *item, *t;
    int i;
    if(num > li->numItems)
	num = li->numItems;

    if (li->boxItems) {
	if(num > 0) {
	    for(i = 0, item = li->boxItems; item->next != NULL && i < num - 1;
		item = item->next, i++);
	    t = item->next;
	    item = item->next = malloc(sizeof(struct items));
	    item->next = t;
	} else {
	    t = li->boxItems;
	    item = li->boxItems = malloc(sizeof(struct items));
	    item->next = t;
	}
    } else {
	item = li->boxItems = malloc(sizeof(struct items));
	item->next = NULL;
    }

    if (li->userHasSetWidth == 0
	&& text && (strlen(text) > li->curWidth))
	li->curWidth = strlen(text);

    item->key = strdup(text?text:"(null)"); item->data = data;
    item->isSelected = 0;
    
    if (li->sb)
	li->sb->left = co->left + li->curWidth + 2;
    if (li->userHasSetWidth == 0)
	co->width = li->curWidth;
    li->numItems++;

    listboxDraw(co);

    return li->numItems;
}

int newtListboxDeleteEntry(newtComponent co, int num) {
    struct listbox * li = co->data;
    int i, widest = 0, t;
    struct items *item, *item2;

    if(num > li->numItems)
	num = li->numItems;

    if (li->boxItems == NULL || li->numItems <= 0)
	return 0;

    if (num <= 1) { 
	item = li->boxItems;
	li->boxItems = item->next;

	/* Fix things up for the width-finding loop near the bottom */
	item2 = li->boxItems;
	widest = strlen(item2?item2->key:"");
    } else {
	for(i = 0, item = li->boxItems; item != NULL && i < num - 1;
	    i++, item = item->next) {
	    if((t = strlen(item->key)) > widest) widest = t;
	    item2 = item;
	}

	if (!item)
	    return -1;

	item2->next = item->next;
    }
    free(item->key);
    free(item);
    li->numItems--;
    if(li->currItem >= num)
	li->currItem--;
    for (item = item2?item2->next:item2; item != NULL; item = item->next)
	if((t = strlen(item->key)) > widest) widest = t;

    /* Adjust the listbox width */
    if (li->userHasSetWidth == 0) {
	co->width = li->curWidth = widest;
	if (li->sb)
		li->sb->left = co->left + widest + 2;
    }

    listboxDraw(co);

    return li->numItems;
}

void newtListboxClear(newtComponent co)
{
    struct listbox * li;
    struct items *anitem, *nextitem;
    if(co == NULL || (li = co->data) == NULL)
	return;
    for(anitem = li->boxItems; anitem != NULL; anitem = nextitem) {
	nextitem = anitem->next;
	free(anitem->key);
	free(anitem);
    }
    li->numItems = li->numSelected = li->currItem = li->startShowItem = 0;
    li->boxItems = NULL;
    if(li->userHasSetWidth == 0)
	li->curWidth = 0;
}

/* If you don't want to get back the text, pass in NULL for the ptr-ptr. Same
   goes for the data. */
void newtListboxGetEntry(newtComponent co, int num, char **text, void **data) {
    struct listbox * li = co->data;
    int i;
    struct items *item;

    if (!li->boxItems || num >= li->numItems) {
	if(text)
	    *text = NULL;
	if(data)
	    *data = NULL;
	return;
    }

    i = 0;
    item = li->boxItems; 
    while (item && i < num) {
	i++, item = item->next;
    }

    if (item) {
	if (text)
	    *text = item->key;
	if (data)
	    *data = item->data; 
    }
}

static void listboxDraw(newtComponent co)
{
    struct listbox * li = co->data;
    struct items *item;
    int i, j;

    if(li->sb)
	li->sb->ops->draw(li->sb);

    if(li->flags & NEWT_FLAG_DOBORDER) {
      if(li->isActive)
	  SLsmg_set_color(NEWT_COLORSET_ACTLISTBOX);
      else
          SLsmg_set_color(NEWT_COLORSET_LISTBOX);

      newtDrawBox(co->left-1, co->top-1, co->width+5, co->height+2, 0);
    }

    SLsmg_set_color(NEWT_COLORSET_LISTBOX);
    
    for(i = 0, item = li->boxItems; item != NULL && i < li->startShowItem;
	i++, item = item->next);

    j = i;

    for (i = 0; item != NULL && i < co->height; i++, item = item->next) {
	if (!item->key) continue;

	newtGotorc(co->top + i, co->left + 1);
	if(j + i == li->currItem) {
	    if(item->isSelected)
		SLsmg_set_color(NEWT_COLORSET_ACTSELLISTBOX);
	    else
		SLsmg_set_color(NEWT_COLORSET_ACTLISTBOX);
	} else if(item->isSelected)
	    SLsmg_set_color(NEWT_COLORSET_SELLISTBOX);
	else
	    SLsmg_set_color(NEWT_COLORSET_LISTBOX);
	    
	SLsmg_write_nstring(item->key, li->curWidth);

    }
    newtGotorc(co->top + (li->currItem - li->startShowItem), co->left);
}

static struct eventResult listboxEvent(newtComponent co, struct event ev) {
    struct eventResult er;
    struct listbox * li = co->data;

    er.result = ER_IGNORED;
	       
    if(ev.when == EV_EARLY || ev.when == EV_LATE) {
	return er;
    }
		       
    switch(ev.event) {
      case EV_KEYPRESS:
	if (!li->isActive) break;

	switch(ev.u.key) {
	  case ' ':
	    if(!(li->flags & NEWT_FLAG_MULTIPLE)) break;
	    newtListboxSelectItem(co, li->currItem);
	    er.result = ER_SWALLOWED;
	    break;

	  case NEWT_KEY_ENTER:
	    if(li->numItems <= 0) break;
	    if(li->flags & NEWT_FLAG_RETURNEXIT)
		er.result = ER_EXITFORM;
	    break;

	  case NEWT_KEY_UP:
	    if(li->numItems <= 0) break;
	    if(li->currItem > 0) {
		li->currItem--;
		if(li->currItem < li->startShowItem)
		    li->startShowItem = li->currItem;
		if(li->sb)
		    newtScrollbarSet(li->sb, li->currItem + 1, li->numItems);
		listboxDraw(co);
	    }
	    if(co->callback) co->callback(co, co->callbackData);
	    er.result = ER_SWALLOWED;
	    break;

	  case NEWT_KEY_DOWN:
	    if(li->numItems <= 0) break;
	    if(li->currItem < li->numItems - 1) {
		li->currItem++;
		if(li->currItem > (li->startShowItem + co->height - 1)) {
		    li->startShowItem = li->currItem - co->height + 1;
		    if(li->startShowItem + co->height > li->numItems)
			li->startShowItem = li->numItems - co->height;
		}
		if(li->sb)
		    newtScrollbarSet(li->sb, li->currItem + 1, li->numItems);
		listboxDraw(co);
	    }
	    if(co->callback) co->callback(co, co->callbackData);
	    er.result = ER_SWALLOWED;
	    break;

	  case NEWT_KEY_PGUP:
	    if(li->numItems <= 0) break;
	    li->startShowItem -= co->height - 1;
	    if(li->startShowItem < 0)
		li->startShowItem = 0;
	    li->currItem -= co->height - 1;
	    if(li->currItem < 0)
		li->currItem = 0;
	    newtListboxRealSetCurrent(co);
	    er.result = ER_SWALLOWED;
	    break;

	  case NEWT_KEY_PGDN:
	    if(li->numItems <= 0) break;
	    li->startShowItem += co->height;
	    if(li->startShowItem > (li->numItems - co->height)) {
		li->startShowItem = li->numItems - co->height;
	    }
	    li->currItem += co->height;
	    if(li->currItem > li->numItems) {
		li->currItem = li->numItems - 1;
	    }
	    newtListboxRealSetCurrent(co);
	    er.result = ER_SWALLOWED;
	    break;

	  case NEWT_KEY_HOME:
	    if(li->numItems <= 0) break;
	    newtListboxSetCurrent(co, 0);
	    er.result = ER_SWALLOWED;
	    break;

	  case NEWT_KEY_END:
	    if(li->numItems <= 0) break;
	    newtListboxSetCurrent(co, li->numItems - 1);
	    er.result = ER_SWALLOWED;
	    break;
	  default:
	    /* keeps gcc quiet */
	}
	break;
	
      case EV_FOCUS:
	li->isActive = 1;
	listboxDraw(co);
	er.result = ER_SWALLOWED;
	break;

      case EV_UNFOCUS:
	li->isActive = 0;
	listboxDraw(co);
	er.result = ER_SWALLOWED;
	break;
    }

    return er;
}

static void listboxDestroy(newtComponent co) {
    struct listbox * li = co->data;
    struct items * item, * nextitem;

    nextitem = item = li->boxItems;

    while (item != NULL) {
	nextitem = item->next;
	free(item->key);
	free(item);
	item = nextitem;
    }

    free(li);
    free(co);
}

