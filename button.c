#include <slang/slang.h>
#include <stdlib.h>
#include <string.h>

#include "newt.h"
#include "newt_pr.h"

struct button {
    char * text;
    char bgColor;
};

static void buttonDrawIt(newtComponent co, int active, int pushed);
static void buttonDrawText(newtComponent co, int active, int pushed);

static void buttonDraw(newtComponent c);
static void buttonDestroy(newtComponent co);
static struct eventResult buttonEvent(struct newtComponent * c, 
				      struct event ev);

static struct componentOps buttonOps = {
    buttonDraw,
    buttonEvent,
    buttonDestroy,
} ;

newtComponent newtButton(int left, int row, char * text) {
    newtComponent co;
    struct button * bu;

    co = malloc(sizeof(*co));
    bu = malloc(sizeof(struct button));
    co->data = bu;

    bu->text = strdup(text);
    co->ops = &buttonOps;

    co->height = 4;
    co->width = strlen(text) + 5;
    co->top = row;
    co->left = left;
    co->takesFocus = 1;

    newtGotorc(co->top, co->left);
    bu->bgColor = (SLsmg_char_at() >> 8) & 0xFF;

    return co;
}

static void buttonDestroy(newtComponent co) {
    struct button * bu = co->data;

    free(bu->text);
    free(bu);
    free(co);
}

static void buttonDraw(newtComponent co) {
    buttonDrawIt(co, 0, 0);
}

static void buttonDrawIt(newtComponent co, int active, int pushed) {
    struct button * bu = co->data;

    SLsmg_set_color(COLORSET_BUTTON);
    if (pushed) {
	SLsmg_set_color(COLORSET_BUTTON);
	newtDrawBox(co->left + 1, co->top + 1, co->width - 1, 3, 0);

	SLsmg_set_color(bu->bgColor);
	newtClearBox(co->left, co->top, co->width, 1);
	newtClearBox(co->left, co->top, 1, co->height);
    } else {
	newtDrawBox(co->left, co->top, co->width - 1, 3, 1);
    }

    buttonDrawText(co, active, pushed);
}

static void buttonDrawText(newtComponent co, int active, int pushed) {
    struct button * bu = co->data;

    if (pushed) pushed = 1;

    if (active)
	SLsmg_set_color(COLORSET_ACTBUTTON);
    else
	SLsmg_set_color(COLORSET_BUTTON);

    newtGotorc(co->top + 1 + pushed, co->left + 1 + pushed);
    SLsmg_write_char(' ');
    SLsmg_write_string(bu->text);
    SLsmg_write_char(' ');
}

static struct eventResult buttonEvent(struct newtComponent * co, 
				      struct event ev) {
    struct eventResult er;

    if (ev.when == EV_NORMAL) {
	switch (ev.event) {
	  case EV_FOCUS:
	    buttonDrawIt(co, 1, 0);
	    er.result = ER_SWALLOWED;
	    break;

	  case EV_UNFOCUS:
	    buttonDrawIt(co, 0, 0);
	    er.result = ER_SWALLOWED;
	    break;

	  case EV_KEYPRESS:
	    if (ev.u.key == ' ' || ev.u.key == '\r') {
		/* look pushed */
		buttonDrawIt(co, 1, 1);
		newtRefresh();
		newtDelay(300000);
		buttonDrawIt(co, 1, 0);
		newtRefresh();
		newtDelay(300000);

		er.result = ER_EXITFORM;
	    } else 
		er.result = ER_IGNORED;
	    break;
	}
    } else 
	er.result = ER_IGNORED;

    return er;
}
