#include <slang.h>
#include <stdlib.h>
#include <string.h>

#include "newt.h"
#include "newt_pr.h"

enum type { CHECK, RADIO };

struct checkbox {
    char * text;
    char * seq;
    char * result;
    newtComponent prevButton, lastButton;
    enum type type;
    char value;
    int active, inactive;
    const void * data;
    int flags;
    int hasFocus;
};

static void cbDraw(newtComponent c);
static void cbDestroy(newtComponent co);
struct eventResult cbEvent(newtComponent co, struct event ev);

static struct componentOps cbOps = {
    cbDraw,
    cbEvent,
    cbDestroy,
    newtDefaultPlaceHandler,
    newtDefaultMappedHandler,
} ;

newtComponent newtRadiobutton(int left, int top, const char * text, int isDefault,
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
    if (co == NULL)
	return NULL;
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
	if (!setMember)
	  return NULL;
	rb = setMember->data;
    }

    return setMember;
}

void newtRadioSetCurrent(newtComponent setMember) {
    struct checkbox * cb = setMember->data;
    struct checkbox * rb;
    newtComponent curr;

    /* find the one that's turned on */
    curr = cb->lastButton;
    rb = curr->data;
    while (curr && rb->value == rb->seq[0]) {
        curr = rb->prevButton;
        if (curr) rb = curr->data;
    }
    if (curr) {
        rb->value = rb->seq[0];
        cbDraw(curr);
    }
    cb->value = cb->seq[1];
    cbDraw(setMember);

    if (setMember->callback)
        setMember->callback(setMember, setMember->callbackData);
}

char newtCheckboxGetValue(newtComponent co) {
    struct checkbox * cb = co->data;

    return cb->value;
}

void newtCheckboxSetValue(newtComponent co, char value) {
    struct checkbox * cb = co->data;

    *cb->result = value;
    cbDraw(co);
}

/* 
 * returns NULL on error.
 * FIXME: Check all calls.
 */
newtComponent newtCheckbox(int left, int top, const char * text, char defValue,
			   const char * seq, char * result) {
    newtComponent co;
    struct checkbox * cb;

    if (!seq) seq = " *";

    co = malloc(sizeof(*co));
    if (co == NULL)
   	return NULL;
    cb = malloc(sizeof(struct checkbox));
    if (cb == NULL) {
	free(co);
	return NULL;
    }
    co->data = cb;
    cb->flags = 0;
    if (result)
	cb->result = result;
    else
	cb->result = &cb->value;

    cb->text = strdup(text);
    cb->seq = strdup(seq);
    cb->type = CHECK;
    cb->hasFocus = 0;
    cb->inactive = COLORSET_CHECKBOX;
    cb->active = COLORSET_ACTCHECKBOX;
    defValue ? (*cb->result = defValue) : (*cb->result = cb->seq[0]);

    co->ops = &cbOps;

    co->callback = NULL;
    co->destroyCallback = NULL;
    co->height = 1;
    co->width = wstrlen(text, -1) + 4;
    co->top = top;
    co->left = left;
    co->takesFocus = 1;
    co->isMapped = 0;

    return co;
}

void newtCheckboxSetFlags(newtComponent co, int flags, enum newtFlagsSense sense) {
    struct checkbox * cb = co->data;
    int row, col;

    cb->flags = newtSetFlags(cb->flags, flags, sense);

    // If the flag just sets a property (eg. NEWT_FLAG_RETURNEXIT),
    // don't redraw, etc. as the component might be 'hidden' and not to
    // be drawn (eg. in a scrolled list)
    if (flags == NEWT_FLAG_RETURNEXIT)
	    return;

    if (!(cb->flags & NEWT_FLAG_DISABLED))
	co->takesFocus = 1;
    else
	co->takesFocus = 0;

    newtGetrc(&row, &col);
    cbDraw(co);
    newtGotorc(row, col);
}

static void cbDraw(newtComponent c) {
    struct checkbox * cb = c->data;

    if (!c->isMapped) return;

    if (cb->flags & NEWT_FLAG_DISABLED) {
	cb->inactive = NEWT_COLORSET_DISENTRY;
	cb->active = NEWT_COLORSET_DISENTRY;
    } else {
	cb->inactive = COLORSET_CHECKBOX;
	cb->active = COLORSET_ACTCHECKBOX;
    }

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

    if (cb->hasFocus)
	SLsmg_set_color(cb->active);

    newtGotorc(c->top, c->left + 1);
    SLsmg_write_char(*cb->result);
    newtGotorc(c->top, c->left + 4);
}

static void cbDestroy(newtComponent co) {
    struct checkbox * cb = co->data;

    free(cb->text);
    free(cb->seq);
    free(cb);
    free(co);
}

struct eventResult cbEvent(newtComponent co, struct event ev) {
    struct checkbox * cb = co->data;
    struct eventResult er;
    const char * cur;

    er.result = ER_IGNORED;

    if (ev.when == EV_NORMAL) {
	switch (ev.event) {
	  case EV_FOCUS:
	    cb->hasFocus = 1;
	    cbDraw(co);
	    er.result = ER_SWALLOWED;
	    break;

	  case EV_UNFOCUS:
	    cb->hasFocus = 0;
	    cbDraw(co);
	    er.result = ER_SWALLOWED;
	    break;

	  case EV_KEYPRESS:
	    if (ev.u.key == ' ') {
		if (cb->type == RADIO) {
		    newtRadioSetCurrent(co);
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
		    cbDraw(co);
		    er.result = ER_SWALLOWED;

		    if (co->callback)
			co->callback(co, co->callbackData);
		} else {
		    er.result = ER_IGNORED;
		}
	    } else if(ev.u.key == NEWT_KEY_ENTER) {
		if (cb->flags & NEWT_FLAG_RETURNEXIT)
			er.result = ER_EXITFORM;
		else
			er.result = ER_IGNORED;
	    } else {
		er.result = ER_IGNORED;
	    }
	    break;
   	  case EV_MOUSE:
	    if (ev.u.mouse.type == MOUSE_BUTTON_DOWN) {
		if (cb->type == RADIO) {
		    newtRadioSetCurrent(co);
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
		    cbDraw(co);
		    er.result = ER_SWALLOWED;

		    if (co->callback)
			co->callback(co, co->callbackData);
		}
	    }
	}
    }

    return er;
}
