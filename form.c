#include <stdarg.h>
#include <stdlib.h>

#include "newt.h"
#include "newt_pr.h"

/****************************************************************************
    These forms handle vertical scrolling of components with a height of 1 
   
    Horizontal scrolling won't work, and scrolling large widgets will fail
    miserably. It shouldn't be too hard to fix either of those if anyone
    cares to. I only use scrolling for listboxes and text boxes though so
    I didn't bother.
*****************************************************************************/

struct element {
    int top, left;		/* actual, not virtual */
    newtComponent co;
};

struct form {
    int numCompsAlloced;
    struct element * elements;
    int numComps;
    int currComp;
    int fixedSize;
    int vertOffset;
};

static void formDraw(newtComponent co);
static void gotoComponent(struct form * form, int newComp);
static struct eventResult formEvent(newtComponent co, struct event ev);

static struct componentOps formOps = {
    formDraw,
    formEvent,
    newtFormDestroy,
} ;

static inline int componentFits(newtComponent co, int compNum) {
    struct form * form = co->data;
    struct element * el = form->elements + compNum;

    if ((co->top + form->vertOffset) > el->top) return 0;
    if ((co->top + form->vertOffset + co->height) <
	    (el->top + el->co->height)) return 0;

    return 1;
}

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

    co->takesFocus = 1;
    co->ops = &formOps;

    form = malloc(sizeof(*form));
    form->numCompsAlloced = 5;
    form->numComps = 0;
    form->currComp = -1;
    form->vertOffset = 0;
    form->fixedSize = 0;
    form->elements = malloc(sizeof(*(form->elements)) * form->numCompsAlloced);

    return co;
}

void newtFormSetSize(newtComponent co, int width, int height) {
    struct form * form = co->data;

    form->fixedSize = 1;
    co->width = width;
    co->height = height;
}

void newtFormAddComponent(newtComponent co, newtComponent newco) {
    struct form * form = co->data;
    int delta;

    if (form->numCompsAlloced == form->numComps) {
	form->numCompsAlloced += 5;
	form->elements = realloc(form->elements, 
			    sizeof(*(form->elements)) * form->numCompsAlloced);
    }

    form->elements[form->numComps].left = newco->left;
    form->elements[form->numComps].top = newco->top;
    form->elements[form->numComps].co = newco;
    form->numComps++;

    if (co->left == -1) {
	co->left = newco->left;
	co->top = newco->top;
	if (!form->fixedSize) {
	    co->width = newco->width;
	    co->height = newco->height;
	}
    } else {
	if (co->left > newco->left) {
	    delta = co->left - newco->left;
	    co->left -= delta;
	    if (!form->fixedSize)
		co->width += delta;
	}

	if (co->top > newco->top) {
	    delta = co->top - newco->top;
	    co->top -= delta;
	    if (!form->fixedSize)
		co->height += delta;
	}

	if (!form->fixedSize) {
	    if ((co->left + co->width) < (newco->left + newco->width)) 
		co->width = (newco->left + newco->width) - co->left;

	    if ((co->top + co->height) < (newco->top + newco->height)) 
		co->height = (newco->top + newco->height) - co->top;
	}
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
    struct element * el;
    int i;

    for (i = 0, el = form->elements; i < form->numComps; i++, el++) {
	/* only draw it if it'll fit on the screen vertically */
	if (componentFits(co, i)) {
	    el->co->top = el->top - form->vertOffset;
	    el->co->ops->draw(el->co);
	} else {
	    el->co->top = -1;		/* tell it not to draw itself */
	}
    }
}

static struct eventResult formEvent(newtComponent co, struct event ev) {
    struct form * form = co->data;
    newtComponent subco = form->elements[form->currComp].co;
    int new;
    struct eventResult er;
    int dir = 0;

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
	dir = -1;
    } else if (er.result == ER_NEXTCOMP) {
	dir = 1;
    }

    if (dir) {
	new = form->currComp;
	do {
	    new += dir;
	    if (new < 0 || new >= form->numComps) return er;
	} while (!form->elements[new].co->takesFocus);

	/* make sure this component is visible */
	if (!componentFits(co, new)) {
	    gotoComponent(form, -1);

	    if (dir < 0) {
		/* make the new component the first one */
		form->vertOffset = form->elements[new].top - co->top;
	    } else {
		/* make the new component the last one */
		form->vertOffset = (form->elements[new].top + 
					form->elements[new].co->height) -
				    (co->top + co->height);
	    }

	    if (form->vertOffset < 0) form->vertOffset = 0;

	    formDraw(co);
	}

	gotoComponent(form, new);
	er.result = ER_SWALLOWED;
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
	subco = form->elements[i].co;
	if (subco->ops->destroy) {
	    subco->ops->destroy(subco);
	} else {
	    if (subco->data) free(subco->data);
	    free(subco);
	}	
    }

    free(form->elements);
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

    return form->elements[form->currComp].co;

}

static void gotoComponent(struct form * form, int newComp) {
    newtComponent co;
    struct event ev;

    if (form->currComp != -1) {
	ev.event = EV_UNFOCUS;
	co = form->elements[form->currComp].co;
	co->ops->event(co, ev);
    }

    form->currComp = newComp;
   
    
    if (form->currComp != -1) {
	ev.event = EV_FOCUS;
	co = form->elements[form->currComp].co;
	co->ops->event(co, ev);
    }
}
