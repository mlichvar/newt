#include <stdarg.h>
#include <stdlib.h>

#include "newt.h"
#include "newt_pr.h"

struct newtForm {
    int numCompsAlloced;
    newtComponent * comps;
    int numComps;
    int currComp;
};

newtForm newtCreateForm(void) {
    newtForm form;

    form = malloc(sizeof(*form));
    form->numCompsAlloced = 5;
    form->numComps = 0;
    form->currComp = -1;
    form->comps = malloc(sizeof(*(form->comps)) * form->numCompsAlloced);

    return form;
}

static void gotoComponent(newtForm form, int newComp);

void newtAddComponentToForm(newtForm form, newtComponent co) {
    if (form->numCompsAlloced == form->numComps) {
	form->numCompsAlloced += 5;
	form->comps = realloc(form->comps, 
			      sizeof(*(form->comps)) * form->numCompsAlloced);
    }

    form->comps[form->numComps++] = co;
}

void newtAddComponentsToForm(newtForm form, ...) {
    va_list ap;
    newtComponent co;

    va_start(ap, form);

    while ((co = va_arg(ap, newtComponent)))
	newtAddComponentToForm(form, co);
 
    va_end(ap);
}

newtComponent newtRunForm(newtForm form) {
    int i;
    newtComponent co;
    struct event ev;
    struct eventResult er;
    int key, new;

    /* first, draw all of the components */
    for (i = 0; i < form->numComps; i++) {
	co = form->comps[i];
	co->ops->draw(co);
    }

    if (form->currComp == -1)
	gotoComponent(form, 0);
  
    do {
	co = form->comps[form->currComp];

	/*newtGotorc(0, 0);*/
	newtRefresh();
	key = newtGetKey(); 

	er.result = ER_IGNORED;

	/* handle system wide events */
	switch (key) {
	  case NEWT_KEY_TAB:
	    new = form->currComp + 1;
	    if (new == form->numComps) new = 0;
	    gotoComponent(form, new);
	    er.result = ER_SWALLOWED;
	    break;
	}

	/* handle component specific events */
	if (er.result == ER_IGNORED) {
	    ev.event = EV_KEYPRESS;
	    ev.u.key = key;
	    er = co->ops->event(co, ev);
	}

	/* handle default events */
	if (er.result == ER_IGNORED) {
	    switch (key) {
	      case NEWT_KEY_UP:
	      case NEWT_KEY_LEFT:
	      case NEWT_KEY_BKSPC:
		if (form->currComp == 0) 
		    new = form->numComps - 1;
		else
		    new = form->currComp - 1;
		  
		gotoComponent(form, new);
		break;

	      case NEWT_KEY_DOWN:
	      case NEWT_KEY_RIGHT:
	      case NEWT_KEY_ENTER:
		new = form->currComp + 1;
		if (new == form->numComps) new = 0;
		gotoComponent(form, new);
		break;
	    }
	}
	
    } while (er.result != ER_EXITFORM);

    newtRefresh();

    return form->comps[form->currComp];
}

/* this also destroys all of the components on the form */
void newtDestroyForm(newtForm form) {
    int i;
    newtComponent co;

    /* first, destroy all of the components */
    for (i = 0; i < form->numComps; i++) {
	co = form->comps[i];
	if (co->ops->destroy) {
	    co->ops->destroy(co);
	} else {
	    if (co->data) free(co->data);
	    free(co);
	}	
    }

    free(form->comps);
    free(form);
}

static void gotoComponent(newtForm form, int newComp) {
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
