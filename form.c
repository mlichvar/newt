#include <stdarg.h>
#include <stdlib.h>

#include "newt.h"
#include "newt_pr.h"

struct form {
    int numCompsAlloced;
    newtComponent * comps;
    int numComps;
    int currComp;
};

static void formDraw(newtComponent co);
static void gotoComponent(struct form * form, int newComp);
static struct eventResult formEvent(newtComponent co, struct event ev);

static struct componentOps formOps = {
    formDraw,
    formEvent,
    newtFormDestroy,
} ;

newtComponent newtForm(void) {
    newtComponent co;
    struct form * form;

    co = malloc(sizeof(*co));
    form = malloc(sizeof(*form));
    co->data = form;
    co->width = 0;
    co->height = 0;
    co->top = -1;
    co->left = -1;

    co->takesFocus = 0;
    co->ops = &formOps;

    form = malloc(sizeof(*form));
    form->numCompsAlloced = 5;
    form->numComps = 0;
    form->currComp = -1;
    form->comps = malloc(sizeof(*(form->comps)) * form->numCompsAlloced);

    return co;
}

void newtFormAddComponent(newtComponent co, newtComponent newco) {
    struct form * form = co->data;
    int delta;

    if (form->numCompsAlloced == form->numComps) {
	form->numCompsAlloced += 5;
	form->comps = realloc(form->comps, 
			      sizeof(*(form->comps)) * form->numCompsAlloced);
    }

    form->comps[form->numComps++] = newco;

    if (co->left == -1) {
	co->left = newco->left;
	co->top = newco->top;
	co->width = newco->width;
	co->height = newco->height;
    } else {
	if (co->left > newco->left) {
	    delta = co->left - newco->left;
	    co->left -= delta;
	    co->width += delta;
	}

	if (co->top > newco->top) {
	    delta = co->top - newco->top;
	    co->top -= delta;
	    co->height += delta;
	}

	if ((co->left + co->width) < (newco->left + newco->width)) 
	    co->left = (newco->left + newco->width) - co->width;

	if ((co->top + co->height) < (newco->top + newco->height)) 
	    co->top = (newco->top + newco->height) - co->height;
    }
}

void newtFormAddComponents(newtComponent co, ...) {
    va_list ap;
    newtComponent subco;

    va_start(ap, co);

    while ((subco = va_arg(ap, newtComponent)))
	newtFormAddComponent(co, subco);
 
    va_end(ap);
}

static void formDraw(newtComponent co) {
    struct form * form = co->data;
    newtComponent subco;
    int i;

    for (i = 0; i < form->numComps; i++) {
	subco = form->comps[i];
	subco->ops->draw(subco);
    }
}

static struct eventResult formEvent(newtComponent co, struct event ev) {
    struct form * form = co->data;
    newtComponent subco = form->comps[form->currComp];
    int new;
    struct eventResult er;

    er.result = ER_IGNORED;

    switch (ev.event) {
      case EV_FOCUS:
      case EV_UNFOCUS:
	er = subco->ops->event(subco, ev);
	break;
     
      case EV_KEYPRESS:
	if (ev.u.key == NEWT_KEY_TAB) {
	    er.result = ER_NEXTCOMP;
	} 

	if (er.result == ER_IGNORED) {
	    /* let the current component handle the event */
	    er = subco->ops->event(subco, ev);
	}

	if (er.result == ER_IGNORED) {
	    /* handle default events */
	    if (er.result == ER_IGNORED) {
		switch (ev.u.key) {
		  case NEWT_KEY_UP:
		  case NEWT_KEY_LEFT:
		  case NEWT_KEY_BKSPC:
		    er.result = ER_PREVCOMP;
		    break;

		  case NEWT_KEY_DOWN:
		  case NEWT_KEY_RIGHT:
		  case NEWT_KEY_ENTER:
		    er.result = ER_NEXTCOMP;
		    break;
		}
	    }
	}
    }

    /* we try and do previous/next actions ourselves if possible */
    if (er.result == ER_PREVCOMP) {
	if (form->currComp > 0) {
	    new = form->currComp - 1;
	    gotoComponent(form, new);
	    er.result = ER_SWALLOWED;
	} 
    } else if (er.result == ER_NEXTCOMP) {
	new = form->currComp + 1;
	if (new < form->numComps) {
	    gotoComponent(form, new);
	    er.result = ER_SWALLOWED;
	}
    }

    return er;
}

/* this also destroys all of the components on the form */
void newtFormDestroy(newtComponent co) {
    int i;
    newtComponent subco;
    struct form * form = co->data;

    /* first, destroy all of the components */
    for (i = 0; i < form->numComps; i++) {
	subco = form->comps[i];
	if (subco->ops->destroy) {
	    subco->ops->destroy(subco);
	} else {
	    if (subco->data) free(subco->data);
	    free(subco);
	}	
    }

    free(form->comps);
    free(form);
    free(co);
}

newtComponent newtRunForm(newtComponent co) {
    struct form * form = co->data;
    struct event ev;
    struct eventResult er;
    int key;

    /* first, draw all of the components */
    formDraw(co);

    if (form->currComp == -1)
	gotoComponent(form, 0);
    else
	gotoComponent(form, form->currComp);
  
    do {
	newtRefresh();
	key = newtGetKey(); 

	ev.event = EV_KEYPRESS;
	ev.u.key = key;
	er = formEvent(co, ev);

	/* EV_NEXTCOMP and EV_PREVCOMP should cause wrapping */
    } while (er.result != ER_EXITFORM);

    newtRefresh();

    return form->comps[form->currComp];

}

static void gotoComponent(struct form * form, int newComp) {
    newtComponent co;
    struct event ev;

    if (form->currComp != -1) {
	ev.event = EV_UNFOCUS;
	co = form->comps[form->currComp];
	co->ops->event(co, ev);
    }

    form->currComp = newComp;
   
    ev.event = EV_FOCUS;
    co = form->comps[form->currComp];
    co->ops->event(co, ev);
}
