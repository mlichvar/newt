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
    int flags;
    int depth;
};

struct CheckboxTree {
    newtComponent sb;
    int curWidth;	/* size of text w/o scrollbar or border*/
    int curHeight;	/* size of text w/o border */
    struct items * itemlist;
    struct items ** flatList, ** currItem, ** firstItem;
    int flatCount;
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
static struct items * findItem(struct items * items, const void * data);
static void buildFlatList(newtComponent co);
static void doBuildFlatList(struct CheckboxTree * ct, struct items * item);
enum countWhat { COUNT_EXPOSED, COUNT_SELECTED };
static int countItems(struct items * item, enum countWhat justExposed);

static struct componentOps ctOps = {
    ctDraw,
    ctEvent,
    ctDestroy,
    ctPlace,
    ctMapped,
} ;

static int countItems(struct items * item, enum countWhat what) {
    int count = 0;

    while (item) {
        if ((!item->branch) || (what == COUNT_EXPOSED))
	    count++;
	if (item->branch && (what == COUNT_EXPOSED && item->selected))
	    count += countItems(item->branch, what);
	item = item->next;
    }

    return count;
}

static void doBuildFlatList(struct CheckboxTree * ct, struct items * item) {
    while (item) {
    	ct->flatList[ct->flatCount++] = item;
	if (item->branch && item->selected) doBuildFlatList(ct, item->branch);
	item = item->next;
    }
}

static void buildFlatList(newtComponent co) {
    struct CheckboxTree * ct = co->data;

    if (ct->flatList) free(ct->flatList);
    ct->flatCount = countItems(ct->itemlist, COUNT_EXPOSED);

    ct->flatList = malloc(sizeof(*ct->flatList) * ct->flatCount);
    ct->flatCount = 0;
    doBuildFlatList(ct, ct->itemlist);;
}

int newtCheckboxTreeAddItem(newtComponent co, 
			    const char * text, const void * data,
			    int flags, int index, ...) {
    va_list argList;
    int numIndexes;
    int * indexes;
    int i;

    va_start(argList, index);
    numIndexes = 0;
    i = index;
    while (i != NEWT_ARG_LAST) {
	numIndexes++;
	i = va_arg(argList, int);
    }

    va_end(argList);

    indexes = alloca(sizeof(*indexes) * (numIndexes + 1));
    va_start(argList, index);
    numIndexes = 0;
    i = index;
    va_start(argList, index);
    while (i != NEWT_ARG_LAST) {
	indexes[numIndexes++] = i;
	i = va_arg(argList, int);
    }
    va_end(argList);

    indexes[numIndexes++] = NEWT_ARG_LAST;

    return newtCheckboxTreeAddArray(co, text, data, flags, indexes);
}

static int doFindItemPath(struct items * items, void * data, int * path, 
			  int * len) {
    int where = 0;

    while (items) {
	if (items->data == data) {
	    if (path) path[items->depth] = where;
	    if (len) *len = items->depth + 1;
	    return 1;
	}

	if (items->branch && doFindItemPath(items->branch, data, path, len)) {
	    if (path) path[items->depth] = where;
	    return 1;
	}

	items = items->next;
	where++;
    }

    return 0;
}

int * newtCheckboxTreeFindItem(newtComponent co, void * data) {
    int len;
    int * path;
    struct CheckboxTree * ct = co->data;

    if (!doFindItemPath(ct->itemlist, data, NULL, &len)) return NULL;

    path = malloc(sizeof(*path) * (len + 1));
    doFindItemPath(ct->itemlist, data, path, NULL);
    path[len] = NEWT_ARG_LAST;

    return path;
}

int newtCheckboxTreeAddArray(newtComponent co, 
			    const char * text, const void * data,
			    int flags, int * indexes) {
    struct items * curList, * newNode, * item;
    struct items ** listPtr = NULL;
    int i, index, numIndexes;
    struct CheckboxTree * ct = co->data;

    printf("here\n");

    numIndexes = 0;
    while (indexes[numIndexes] != NEWT_ARG_LAST) numIndexes++;

    printf("there\n");

    if (!ct->itemlist) {
	if (numIndexes > 1) return -1;

    	ct->itemlist = malloc(sizeof(*ct->itemlist));
    	item = ct->itemlist;
	item->prev = NULL;
	item->next = NULL;
    } else {
	curList = ct->itemlist;
	listPtr = &ct->itemlist;

	i = 0;
	index = indexes[i];
	while (i < numIndexes) {
	    item = curList;

	    if (index == NEWT_ARG_APPEND) {
	    	item = NULL;
	    } else {
		while (index && item) 
		    item = item->next, index--;
	    }

	    i++;
	    if (i < numIndexes) {
		curList = item->branch;
		listPtr = &item->branch;
		if (!curList && (i + 1 != numIndexes)) return -1;

		index = indexes[i];
	    }
	}

	if (!curList) { 			/* create a new branch */
	    item = malloc(sizeof(*curList->prev));
	    item->next = item->prev = NULL;
	    *listPtr = item;
	} else if (!item) {			/* append to end */
	    item = curList;
	    while (item->next) item = item->next;
	    item->next = malloc(sizeof(*curList->prev));
	    item->next->prev = item;
	    item = item->next;
	    item->next = NULL;
	} else { 
	    newNode = malloc(sizeof(*newNode));
	    newNode->prev = item->prev;
	    newNode->next = item;

	    if (item->prev) item->prev->next = newNode;
	    item->prev = newNode;
	    item = newNode;
	    if (!item->prev) *listPtr = item;
	}
    }
    	
    item->text = strdup(text);
    item->data = data;
    if (flags & NEWT_FLAG_SELECTED) {
    	item->selected = 1;
    } else {
	item->selected = 0;
    }
    item->flags = flags;
    item->branch = NULL;
    item->depth = numIndexes - 1;

    i = 4;

    if ((strlen(text) + i + ct->pad) > co->width) {
	co->width = strlen(text) + i + ct->pad;
    }

    return 0;
}

static struct items * findItem(struct items * items, const void * data) {
    struct items * i;

    while (items) {
	if (items->data == data) return items;
    	if (items->branch) {
	    i = findItem(items->branch, data);
	    if (i) return i;
	}

	items = items->next;
    }

    return NULL;
}

static void listSelected(struct items * items, int * num, void ** list) {
    while (items) {
        if (items->selected && !items->branch)
	    list[(*num)++] = items->data;
	if (items->branch)
	    listSelected(items->branch, num, list);
	items = items->next;
    }
}

void ** newtCheckboxTreeGetSelection(newtComponent co, int *numitems)
{
    struct CheckboxTree * ct;
    void **retval;

    if(!co || !numitems) return NULL;

    ct = co->data;

    *numitems = countItems(ct->itemlist, COUNT_SELECTED);
    if (!*numitems) return NULL;
    
    retval = malloc(*numitems * sizeof(void *));
    *numitems = 0;
    listSelected(ct->itemlist, numitems, retval);

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
    ct->itemlist = NULL;
    ct->firstItem = NULL;
    ct->currItem = NULL;
    ct->flatList = NULL;
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
    struct items ** currItem;
    
    if (!item)
	return 1;
    
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

    if (item->branch) {
    	currItem = ct->currItem;

    	buildFlatList(co);

    	ct->currItem = ct->flatList;
	while (*ct->currItem != *currItem) ct->currItem++;
    }

    return 0;
}


static void ctDraw(newtComponent co) {
    struct CheckboxTree * ct = co->data;
    struct items ** item; 
    int i, curr, j;

    if (!co->isMapped) return ;

    if (!ct->firstItem) {
	buildFlatList(co);
	ct->firstItem = ct->currItem = ct->flatList;
    }

    item = ct->firstItem;
    
    i = 0;
    while (*item && i < co->height) {
	newtGotorc(co->top + i, co->left);
	if (*item == *ct->currItem) {
	    SLsmg_set_color(NEWT_COLORSET_ACTLISTBOX);
	} else
	    SLsmg_set_color(NEWT_COLORSET_LISTBOX);

	for (j = 0; j < (*item)->depth; j++)
	    SLsmg_write_string("   ");

	if ((*item)->branch) {
	    if ((*item)->selected) 
		SLsmg_write_string("<-> ");
	    else
		SLsmg_write_string("<+> ");
	} else {
	    if ((*item)->selected) 
		SLsmg_write_string("[*] ");
	    else
		SLsmg_write_string("[ ] ");
	}

	SLsmg_write_nstring((*item)->text, co->width - 4 - 
					   (3 * (*item)->depth));
	item++;
	i++;
    }
    
    if(ct->sb) {
	newtScrollbarSet(ct->sb, ct->currItem - ct->flatList, 
			 ct->flatCount - 1);
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
    struct items ** listEnd, ** lastItem;

    er.result = ER_IGNORED;

    if(ev.when == EV_EARLY || ev.when == EV_LATE) {
	return er;
    }

    switch(ev.event) {
    case EV_KEYPRESS:
	switch(ev.u.key) {
	case ' ':
	case NEWT_KEY_ENTER:
	    if (*ct->currItem) {
		ctSetItem(co, *ct->currItem, NEWT_FLAGS_TOGGLE);
		ctDraw(co);
		er.result = ER_SWALLOWED;
	    }
	    break;
	case NEWT_KEY_DOWN:
	    if ((ct->currItem - ct->flatList + 1) < ct->flatCount) {
		ct->currItem++;

		er.result = ER_SWALLOWED;

		if (ct->currItem - ct->firstItem >= co->height) 
		    ct->firstItem++;

		ctDraw(co);
	    }
	    break;
	case NEWT_KEY_UP:
	    if (ct->currItem != ct->flatList) {
		ct->currItem--;
		er.result = ER_SWALLOWED;

		if (ct->currItem < ct->firstItem)
		    ct->firstItem = ct->currItem;
		    
		ctDraw(co);
	    }
	    break;
	case NEWT_KEY_PGUP:
	    if (ct->firstItem - co->height < ct->flatList) {
	    	ct->firstItem = ct->currItem = ct->flatList;
	    } else {
		ct->currItem -= co->height;
		ct->firstItem -= co->height;
	    }

	    ctDraw(co);
	    er.result = ER_SWALLOWED;
	    break;
	case NEWT_KEY_PGDN:
	    listEnd = ct->flatList + ct->flatCount - 1;
	    lastItem = ct->firstItem + co->height - 1;

	    if (lastItem + co->height > listEnd) {
	    	ct->firstItem = listEnd - co->height + 1;
		ct->currItem = listEnd;
	    } else {
	    	ct->currItem += co->height;
		ct->firstItem += co->height;
	    }

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
