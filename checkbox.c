#include <slang/slang.h>
#include <stdlib.h>
#include <string.h>

#include "newt.h"
#include "newt_pr.h"

enum type { CHECK, RADIO, LISTITEM };

struct checkbox {
    char * text;
    char * seq;
    char * result;
    newtComponent prevButton, lastButton;
    enum type type;
    char value;
    int active, inactive;
    void * data;
};

static void cbDrawIt(newtComponent c, int active);
static void makeActive(newtComponent co);

static void cbDraw(newtComponent c);
static void cbDestroy(newtComponent co);
struct eventResult cbEvent(struct newtComponent * co, struct event ev);

static struct componentOps cbOps = {
    cbDraw,
    cbEvent,
    cbDestroy,
} ;

newtComponent newtListitem(int left, int top, char * text, int isDefault,
			      newtComponent prevItem, void * data) {
    newtComponent co;
    struct checkbox * li;

    co = newtRadiobutton(left, top, text, isDefault, prevItem);
    li = co->data;
    li->type = LISTITEM;

    li->inactive = COLORSET_LISTBOX;
    li->active = COLORSET_ACTLISTBOX;
    li->data = data;

    return co;
}

void * newtListitemGetData(newtComponent co) {
    struct checkbox * rb = co->data;

    return rb->data;
}

void newtListitemSet(newtComponent co, char * text) {
    struct checkbox * li = co->data;

    free(li->text);
    li->text = strdup(text);

    if (strlen(text) + 4 > co->width)
	co->width = strlen(text) + 4;
}

newtComponent newtRadiobutton(int left, int top, char * text, int isDefault,
			      newtComponent prevButton) {
    newtComponent co;
    newtComponent curr;
    struct checkbox * rb;
    char initialValue;

    if (isDefault)
	initialValue = '*';
    else
	initialValue = ' ';

    co = newtCheckbox(left, top, text, initialValue, " *", NULL);
    rb = co->data;
    rb->type = RADIO;

    rb->prevButton = prevButton;

    for (curr = co; curr; curr = rb->prevButton) {
	rb = curr->data;
	rb->lastButton = co;
    }

    return co;
}

newtComponent newtRadioGetCurrent(newtComponent setMember) {
    struct checkbox * rb = setMember->data;
    
    setMember = rb->lastButton;
    rb = setMember->data;

    while (rb && rb->value != '*') {
	setMember = rb->prevButton;
	rb = setMember->data;
    }

    return setMember;
}

newtComponent newtCheckbox(int left, int top, char * text, char defValue,
			   char * seq, char * result) {
    newtComponent co;
    struct checkbox * cb;

    if (!seq) seq = " *";

    co = malloc(sizeof(*co));
    cb = malloc(sizeof(struct checkbox));
    co->data = cb;

    if (result)
	cb->result = result;
    else
	cb->result = &cb->value;

    cb->text = strdup(text);
    cb->seq = strdup(seq);
    cb->type = CHECK;
    cb->inactive = COLORSET_CHECKBOX;
    cb->active = COLORSET_ACTCHECKBOX;
    defValue ? (*cb->result = defValue) : (*cb->result = cb->seq[0]);

    co->ops = &cbOps;

    co->callback = NULL;
    co->height = 1;
    co->width = strlen(text) + 4;
    co->top = top;
    co->left = left;
    co->takesFocus = 1;

    return co;
}

static void cbDraw(newtComponent c) {
    cbDrawIt(c, 0);
}

static void cbDrawIt(newtComponent c, int active) {
    struct checkbox * cb = c->data;

    if (c->top == -1) return;

    if (cb->type == LISTITEM && *cb->result != ' ')
	SLsmg_set_color(cb->active);
    else
	SLsmg_set_color(cb->inactive);

    newtGotorc(c->top, c->left);

    switch (cb->type) {
      case RADIO:
	SLsmg_write_string("( ) ");
	break;

      case CHECK:
	SLsmg_write_string("[ ] ");
	break;

      default:
	break;
    }

    SLsmg_write_string(cb->text);

    if (active) 
	SLsmg_set_color(cb->active);

    if (cb->type != LISTITEM) {
	newtGotorc(c->top, c->left + 1);
	SLsmg_write_char(*cb->result);
    }
}

static void cbDestroy(newtComponent co) {
    struct checkbox * cb = co->data;

    free(cb->text);
    free(cb->seq);
    free(cb);
    free(co);
}

struct eventResult cbEvent(struct newtComponent * co, struct event ev) {
    struct checkbox * cb = co->data;
    struct eventResult er;
    char * cur;

    if (ev.when == EV_NORMAL) {
	switch (ev.event) {
	  case EV_FOCUS:
	    if (cb->type == LISTITEM)
		makeActive(co);
	    else 
		cbDrawIt(co, 1);
	    
	    er.result = ER_SWALLOWED;
	    break;

	  case EV_UNFOCUS:
	    cbDrawIt(co, 0);
	    er.result = ER_SWALLOWED;
	    break;

	  case EV_KEYPRESS:
	    if (ev.u.key == ' ') {
		if (cb->type == RADIO) {
		    makeActive(co);
		} else if (cb->type == CHECK) {
		    cur = strchr(cb->seq, *cb->result);
		    if (!cur)
			*cb->result = *cb->seq;
		    else {
			cur++;
			if (! *cur) 
			    *cb->result = *cb->seq;
			else
			    *cb->result = *cur;
		    }
		    cbDrawIt(co, 1);
		    er.result = ER_SWALLOWED;
		} else {
		    er.result = ER_IGNORED;
		}
	    } else {
		er.result = ER_IGNORED;
	    }
	    break;
	}
    } else 
	er.result = ER_IGNORED;

    if (er.result == ER_SWALLOWED && co->callback)
	co->callback(co, co->callbackData);

    return er;
}

static void makeActive(newtComponent co) {
    struct checkbox * cb = co->data;
    struct checkbox * rb;
    newtComponent curr;

    /* find the one that's turned off */
    curr = cb->lastButton;
    rb = curr->data;
    while (curr && rb->value == rb->seq[0]) {
	curr = rb->prevButton;
	if (curr) rb = curr->data;
    }
    if (curr) {
	rb->value = rb->seq[0];
	cbDrawIt(curr, 0);
    } 
    cb->value = cb->seq[1];
    cbDrawIt(co, 1);
}
