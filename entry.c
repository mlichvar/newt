#include <ctype.h>
#include <slang/slang.h>
#include <stdlib.h>

#include "newt.h"
#include "newt_pr.h"

struct entry {
    int flags;
    char * buf;
    char ** resultPtr;
    int bufAlloced;
    int bufUsed;		/* amount of the buffer that's been used */
    int cursorPosition; 	/* cursor *in the string* on on screen */
    int firstChar;		/* first character position being shown */
    void * callbackData;
    newtCallback callback;
};

static void entryDraw(newtComponent co);
static void entryDestroy(newtComponent co);
static struct eventResult entryEvent(struct newtComponent * co, 
			             struct event ev);

static struct eventResult entryHandleKey(struct newtComponent * co, int key);

static struct componentOps entryOps = {
    entryDraw,
    entryEvent,
    entryDestroy,
} ;

void newtEntrySet(newtComponent co, char * value, int cursorAtEnd) {
    struct entry * en = co->data;

    if ((strlen(value) + 1) > en->bufAlloced) {
	free(en->buf);
	en->bufAlloced = strlen(value) + 1;
	en->buf = malloc(en->bufAlloced);
	*en->resultPtr = en->buf;
    }
    strcpy(en->buf, value);
    en->bufUsed = strlen(value);
    en->firstChar = 0;
    if (cursorAtEnd)
	en->cursorPosition = en->bufUsed;
    else
	en->cursorPosition = 0;

    entryDraw(co);
} ;

newtComponent newtEntry(int left, int top, char * initialValue, int width,
			char ** resultPtr, int flags) {
    newtComponent co;
    struct entry * en;

    co = malloc(sizeof(*co));
    en = malloc(sizeof(struct entry));
    co->data = en;

    co->top = top;
    co->left = left;
    co->height = 1;
    co->width = width;
    co->takesFocus = 1;

    co->ops = &entryOps;

    en->flags = flags;
    en->cursorPosition = 0;
    en->firstChar = 0;
    en->bufUsed = 0;
    en->bufAlloced = width + 1;
    en->callback = NULL;

    if (initialValue && strlen(initialValue) > width) {
	en->bufAlloced = strlen(initialValue) + 1;
    }
    en->buf = malloc(en->bufAlloced);
    *resultPtr = en->buf;
    en->resultPtr = resultPtr;
  
    memset(en->buf, 0, en->bufAlloced);
    if (initialValue) {
	strcpy(en->buf, initialValue);
	en->bufUsed = strlen(initialValue);
    }

    return co;
}

void newtEntryAddCallback(newtComponent co, newtCallback f, void * data) {
    struct entry * en = co->data;

    en->callback = f;
    en->callbackData = data;
}

static void entryDraw(newtComponent co) {
    struct entry * en = co->data;
    int i;
    char * chptr;
    int len;

    if (co->top == -1) return;
 
    if (en->flags & NEWT_ENTRY_HIDDEN) {
	newtGotorc(co->top, co->left);
	SLsmg_set_color(COLORSET_ENTRY);
	for (i = 0; i < co->width; i++)
	    SLsmg_write_char('_');
	newtGotorc(co->top, co->left);

	return;
    }

    newtGotorc(co->top, co->left);
    SLsmg_set_color(COLORSET_ENTRY);

    if (en->cursorPosition < en->firstChar) {
	/* scroll to the left */
	en->firstChar = en->cursorPosition;
    } else if ((en->firstChar + co->width) <= en->cursorPosition) {
	/* scroll to the right */
	en->firstChar = en->cursorPosition - co->width + 1;
    }

    chptr = en->buf + en->firstChar;
    len = strlen(chptr);
 
    if (len <= co->width) {
	i = len;
	SLsmg_write_string(chptr);
	while (i < co->width) {
	    SLsmg_write_char('_');
	    i++;
	}
    } else {
	SLsmg_write_nstring(chptr, co->width);
    }

    newtGotorc(co->top, co->left + (en->cursorPosition - en->firstChar));
}

static void entryDestroy(newtComponent co) {
    struct entry * en = co->data;

    free(en->buf);
    free(en);
    free(co);
}

static struct eventResult entryEvent(struct newtComponent * co, 
				     struct event ev) {
    struct entry * en = co->data;
    struct eventResult er;

    if (ev.when == EV_NORMAL) {
	switch (ev.event) {
	  case EV_FOCUS:
	    /*SLtt_set_cursor_visibility(0);*/
	    newtGotorc(co->top, co->left + 
			        (en->cursorPosition - en->firstChar));
	    er.result = ER_SWALLOWED;
	    break;

	  case EV_UNFOCUS:
	    /*SLtt_set_cursor_visibility(1);*/
	    newtGotorc(0, 0);
	    er.result = ER_SWALLOWED;
	    if (en->callback) en->callback(co, en->callbackData);
	    break;

	  case EV_KEYPRESS:
	    er = entryHandleKey(co, ev.u.key);
	    break;
	}
    } else
	er.result = ER_IGNORED;

    return er;
}

static struct eventResult entryHandleKey(struct newtComponent * co, int key) {
    struct entry * en = co->data;
    struct eventResult er;
    char * chptr, * insPoint;

    er.result = ER_SWALLOWED;
    switch (key) {
      case '\001':				/* ^A */
      case NEWT_KEY_HOME:
	en->cursorPosition = 0;
	break;

      case '\005':				/* ^E */
      case NEWT_KEY_END:
	en->cursorPosition = en->bufUsed;
	break;

      case '\013':				/* ^K */
	en->bufUsed = en->cursorPosition;
	memset(en->buf + en->bufUsed, 0, en->bufAlloced - en->bufUsed);
	break;

      case '\002':				/* ^B */
      case NEWT_KEY_LEFT:
	if (en->cursorPosition)
	    en->cursorPosition--;
	break;

      case '\004':
      case NEWT_KEY_DELETE:
	chptr = en->buf + en->cursorPosition;
	if (*chptr) {
	    chptr++;
	    while (*chptr) {
		*(chptr - 1) = *chptr;
		chptr++;
	    }
	    *(chptr - 1) = '\0';
	    en->bufUsed--;
	}
	break;

      case NEWT_KEY_BKSPC:
	if (en->cursorPosition) {
	    /* if this isn't true, there's nothing to erase */
	    chptr = en->buf + en->cursorPosition;
	    en->bufUsed--;
	    en->cursorPosition--;
	    while (*chptr) {
		*(chptr - 1) = *chptr;
		chptr++;
	    }
	    *(chptr - 1) = '\0';
	}
	break;

      case '\006':				/* ^B */
      case NEWT_KEY_RIGHT:
	if (en->cursorPosition < en->bufUsed)
	    en->cursorPosition++;
	break;

      default:
	if ((key >= 0x20 && key <= 0x7e) || (key >= 0xa0 && key <= 0xff)) {
	    if (!(en->flags & NEWT_ENTRY_SCROLL) && en->bufUsed == co->width) {
		SLtt_beep();
		break;
	    } 
	
	    if ((en->bufUsed + 1) == en->bufAlloced) {
		en->bufAlloced += 20;
		en->buf = realloc(en->buf, en->bufAlloced);
		*en->resultPtr = en->buf;
		memset(en->buf + en->bufUsed + 1, 0, 20);
	    }

	    if (en->cursorPosition == en->bufUsed) {
		en->bufUsed++;
	    } else {
		/* insert the new character */

		/* chptr is the last character in the string */
		chptr = (en->buf + en->bufUsed) - 1;
		if ((en->bufUsed + 1) == en->bufAlloced) {
		    /* this string fills the buffer, so clip it */
		    chptr--;
		} else 
		    en->bufUsed++;

		insPoint = en->buf + en->cursorPosition;

		while (chptr >= insPoint) {
		    *(chptr + 1) = *chptr;
		    chptr--;
		}

	    }
		
	    en->buf[en->cursorPosition++] = key;
	} else {
	    er.result = ER_IGNORED;
	}
    } 

    entryDraw(co);

    return er;
}
