#include <slang.h>
#include <stdlib.h>
#include <string.h>

#include "newt.h"
#include "newt_pr.h"

struct items {
    char * text;
    const void *data;
    unsigned char selected;
    struct items *next;
    struct items *prev;
    struct items *branch;
};

struct CheckboxTree {
    newtComponent sb;
    int curWidth;	/* size of text w/o scrollbar or border*/
    int curHeight;	/* size of text w/o border */
    int sbAdjust;
    int bdxAdjust, bdyAdjust;
    int numBoxes, numSelected;
    int userHasSetWidth;
    struct items * currItem, * firstItem, * itemlist;
    int currPos;
    int isActive; 
    int grow;
    int flags;
    int pad;
    char * seq;
    char * result;
};

static void ctDraw(newtComponent c);
static void ctDestroy(newtComponent co);
static void ctPlace(newtComponent co, int newLeft, int newTop);
struct eventResult ctEvent(newtComponent co, struct event ev);
static void ctMapped(newtComponent co, int isMapped);

static struct componentOps ctOps = {
    ctDraw,
    ctEvent,
    ctDestroy,
    ctPlace,
    ctMapped,
} ;

int newtCheckboxTreeAppend(newtComponent co, int selected,
			   const char * text,
			   const void * data) {
    struct CheckboxTree * ct = co->data;
    struct items *item, *prev;

    if(ct->itemlist) {
	for (item = ct->itemlist; item->next != NULL; item = item->next);
	prev = item;
	item = item->next = malloc(sizeof(struct items));
	item->prev = prev;
    } else {
	item = ct->itemlist = ct->firstItem =
	    ct->currItem = malloc(sizeof(struct items));
	item->prev = NULL;
    }

    if ((strlen(text) + 4 + ct->pad) > co->width) {
	co->width = strlen(text) + 4 + ct->pad;
    }
    
    item->text = strdup(text); item->data = data; item->next = NULL;
    item->selected = selected;
    ct->numSelected += selected;
    ct->numBoxes++;
    return 0;
}

void ** newtCheckboxTreeGetSelection(newtComponent co, int *numitems)
{
    struct CheckboxTree * ct;
    int i;
    void **retval;
    struct items *item;

    if(!co || !numitems) return NULL;

    ct = co->data;
    if(!ct || !ct->numSelected) {
	*numitems = 0;
	return NULL;
    }
    
    retval = malloc(ct->numSelected * sizeof(void *));
    for(i = 0, item = ct->itemlist; item != NULL;
	item = item->next)
	if(item->selected)
	    retval[i++] = (void *)item->data;
    *numitems = ct->numSelected;
    return retval;
}

newtComponent newtCheckboxTree(int left, int top, int height, int flags) {
    newtComponent co;
    struct CheckboxTree * ct;

    co = malloc(sizeof(*co));
    ct = malloc(sizeof(struct CheckboxTree));
    co->data = ct;
    co->ops = &ctOps;
    co->takesFocus = 1;
    co->height = height;
    co->width = 0;
    co->isMapped = 0;
    ct->numSelected = 0;
    ct->currPos = 0;
    ct->itemlist = NULL;
    ct->numBoxes = 0;
    if (flags & NEWT_FLAG_SCROLL) {
	ct->sb = newtVerticalScrollbar(left, top, height,
				       COLORSET_LISTBOX, COLORSET_ACTLISTBOX);
	ct->pad = 2;
    } else {
	ct->sb = NULL;
	ct->pad = 0;
    }
    
    return co;
}

static void ctMapped(newtComponent co, int isMapped) {
    struct CheckboxTree * ct = co->data;

    co->isMapped = isMapped;
    if (ct->sb)
	ct->sb->ops->mapped(ct->sb, isMapped);
}

static void ctPlace(newtComponent co, int newLeft, int newTop) {
    struct CheckboxTree * ct = co->data;

    co->top = newTop;
    co->left = newLeft;

    if (ct->sb)
	ct->sb->ops->place(ct->sb, co->left + co->width - 1, co->top);
}

int ctSetItem(newtComponent co, struct items *item, enum newtFlagsSense sense)
{
    struct CheckboxTree * ct = co->data;
    
    if (!item)
	return 1;
    
    if (item->selected)
	ct->numSelected--;

    switch(sense) {
	case NEWT_FLAGS_RESET:
	    item->selected = 0;
	    break;
	case NEWT_FLAGS_SET:
	    item->selected = 1;
	    break;
	case NEWT_FLAGS_TOGGLE:
	    item->selected = !item->selected;
	    break;
    }

    if (item->selected)
	ct->numSelected++;
    
    return 0;
}


static void ctDraw(newtComponent co) {
    struct CheckboxTree * ct = co->data;
    struct items * item; 
    int i, curr, pos = 0;
    
    item = ct->itemlist;
    while (item && item != ct->firstItem) {
	item = item->next;
	pos++;
    }
    
    i = 0;
    while (item && i < co->height) {
	newtGotorc(co->top + i, co->left);
	if (item == ct->currItem) {
	    SLsmg_set_color(NEWT_COLORSET_ACTLISTBOX);
	    ct->currPos = curr = i;
	} else
	    SLsmg_set_color(NEWT_COLORSET_LISTBOX);
	SLsmg_write_string("[");
	SLsmg_write_string(item->selected ? "*" : " ");
	SLsmg_write_string("] ");
	SLsmg_write_nstring(item->text, co->width - 4);
	item = item->next;
	i++;
    }
    
    if(ct->sb) {
	newtScrollbarSet(ct->sb, pos + curr + 1, ct->numBoxes);
	ct->sb->ops->draw(ct->sb);
    }

    newtGotorc(co->top + curr, co->left + 1);
}

static void ctDestroy(newtComponent co) {
    struct CheckboxTree * ct = co->data;
    struct items * item, * nextitem;

    nextitem = item = ct->itemlist;

    while (item != NULL) {
	nextitem = item->next;
	free(item->text);
	free(item);
	item = nextitem;
    }

    free(ct);
    free(co);
}

struct eventResult ctEvent(newtComponent co, struct event ev) {
    struct CheckboxTree * ct = co->data;
    struct eventResult er;
    struct items * item;
    int i;

    er.result = ER_IGNORED;

    if(ev.when == EV_EARLY || ev.when == EV_LATE) {
	return er;
    }

    switch(ev.event) {
    case EV_KEYPRESS:
	switch(ev.u.key) {
	case ' ':
	case NEWT_KEY_ENTER:
	    if (ct->currItem) {
		ctSetItem(co, ct->currItem, NEWT_FLAGS_TOGGLE);
		ctDraw(co);
		er.result = ER_SWALLOWED;
	    }
	    break;
	case NEWT_KEY_DOWN:
	    if (ct->currItem && ct->currItem->next) {
		ct->currItem = ct->currItem->next;
		er.result = ER_SWALLOWED;
		/* Check to see if our new position (currPos + 1) is past
		   the end of the list.  If so, scroll up. */
		if (ct->currPos + 1 >= co->height) {
		    if (ct->firstItem->next)
			ct->firstItem = ct->firstItem->next;
		    ct->currPos = co->height;
		} else
		    ct->currPos++;
		ctDraw(co);
	    }
	    break;
	case NEWT_KEY_UP:
	    if (ct->currItem->prev) {
		ct->currItem = ct->currItem->prev;
		er.result = ER_SWALLOWED;
		/* Check to see if our new position (currPos - 1) is past
		   the beginning of the list.  If so, scroll down. */
		if (ct->currPos <= 0) {
		    if (ct->firstItem->prev)
			ct->firstItem = ct->firstItem->prev;
		    ct->currPos = 0;
		} else
		    ct->currPos--;
		ctDraw(co);
	    }
	    break;
	case NEWT_KEY_PGUP:
	    item = ct->currItem;

	    for (i = 0; i < co->height; i++) {
		if (item->prev)
		    item = item->prev;
		else
		    break;
	    }
	    ct->currItem = ct->firstItem = item;
	    ct->currPos = 0;
	    ctDraw(co);
	    er.result = ER_SWALLOWED;
	    break;
	case NEWT_KEY_PGDN:
	    item = ct->currItem;

	    for (i = 0; i < co->height; i++) {
		if (item->next)
		    item = item->next;
		else
		    break;
	    }
	    ct->currItem = item;
	    ct->currPos = co->height;
				    
	    /* back up the starting item to the current position */
	    for (i = 0; i < co->height - 1; i++) {
		if (item->prev)
		    item = item->prev;
		else
		    break;
	    }
	    ct->firstItem = item;
	    ctDraw(co);
	    er.result = ER_SWALLOWED;
	    break;
	}
    case EV_FOCUS:
	ctDraw(co);
	er.result = ER_SWALLOWED;
	break;
	
    case EV_UNFOCUS:
	ctDraw(co);
	er.result = ER_SWALLOWED;
	break;
    default:
	break;
    }

    return er;
}
