#include <slang/slang.h>
#include <stdlib.h>
#include <string.h>

#include "newt.h"
#include "newt_pr.h"

struct checkbox {
    char * text;
    char * seq;
    char * result;
    newtComponent prevButton, lastButton;
    char isRadio;
    char value;
};

static void cbDrawIt(newtComponent c, int active);

static void cbDraw(newtComponent c);
static void cbDestroy(newtComponent co);
struct eventResult cbEvent(struct newtComponent * co, struct event ev);

static struct componentOps cbOps = {
    cbDraw,
    cbEvent,
    cbDestroy,
} ;

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
    rb->isRadio = 1;

    rb->prevButton = prevButton;

    for (curr = co; curr; curr = rb->prevButton) {
	rb = curr->data;
	rb->lastButton = co;
    }

    return co;
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
    defValue ? (*cb->result = defValue) : (*cb->result = cb->seq[0]);
    cb->isRadio = 0;

    co->ops = &cbOps;

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

    SLsmg_set_color(COLORSET_CHECKBOX);

    newtGotorc(c->top, c->left);

    if (cb->isRadio) 
	SLsmg_write_string("( ) ");
    else
	SLsmg_write_string("[ ] ");

    SLsmg_write_string(cb->text);

    if (active) 
	SLsmg_set_color(COLORSET_ACTCHECKBOX);

    newtGotorc(c->top, c->left + 1);
    SLsmg_write_char(*cb->result);
}

static void cbDestroy(newtComponent co) {
    struct checkbox * cb = co->data;

    free(cb->text);
    free(cb->seq);
    free(cb);
    free(co);
}

struct eventResult cbEvent(struct newtComponent * co, struct event ev) {
    struct eventResult er;
    struct checkbox * cb = co->data;
    struct checkbox * rb;
    newtComponent curr;
    char * cur;

    switch (ev.event) {
      case EV_FOCUS:
	cbDrawIt(co, 1);
	er.result = ER_SWALLOWED;
	break;

      case EV_UNFOCUS:
	cbDrawIt(co, 0);
	er.result = ER_SWALLOWED;
	break;

      case EV_KEYPRESS:
	if (ev.u.key == ' ' || ev.u.key == '\r') {
	    if (cb->isRadio) {
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
	    } else {
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
	    }
	} else {
	    er.result = ER_IGNORED;
	}
    }

   return er;
}
