#include "config.h"

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include <ctype.h>
#include <slang.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

#include "newt.h"
#include "newt_pr.h"

struct entry {
    int flags;
    char * buf;
    const char ** resultPtr;
    int bufAlloced;
    int bufUsed;		/* amount of the buffer that's been used */
    int cursorPosition; 	/* cursor *in the string* on on screen */
    int firstChar;		/* first character position being shown */
    newtEntryFilter filter;
    void * filterData;
    int cs;
    int csDisabled;
};

static int previous_char(const char *buf, int pos);
static int next_char(const char *buf, int pos);
static void entryDraw(newtComponent co);
static void entryDestroy(newtComponent co);
static struct eventResult entryEvent(newtComponent co,
			             struct event ev);

static struct eventResult entryHandleKey(newtComponent co, int key);

static struct componentOps entryOps = {
    entryDraw,
    entryEvent,
    entryDestroy,
    newtDefaultPlaceHandler,
    newtDefaultMappedHandler,
} ;

void newtEntrySet(newtComponent co, const char * value, int cursorAtEnd) {
    struct entry * en = co->data;

    if ((strlen(value) + 1) > (unsigned int)en->bufAlloced) {
	free(en->buf);
	en->bufAlloced = strlen(value) + 1;
	en->buf = malloc(en->bufAlloced);
	if (en->resultPtr) *en->resultPtr = en->buf;
    }
    memset(en->buf, 0, en->bufAlloced);		/* clear the buffer */
    strcpy(en->buf, value);
    en->bufUsed = strlen(value);
    en->firstChar = 0;
    if (cursorAtEnd)
	en->cursorPosition = en->bufUsed;
    else
	en->cursorPosition = 0;

    entryDraw(co);
} ;

newtComponent newtEntry(int left, int top, const char * initialValue, int width,
			const char ** resultPtr, int flags) {
    newtComponent co;
    struct entry * en;

    co = malloc(sizeof(*co));
    en = malloc(sizeof(struct entry));
    co->data = en;

    co->top = top;
    co->left = left;
    co->height = 1;
    co->width = width;
    co->isMapped = 0;
    co->callback = NULL;
    co->destroyCallback = NULL;

    co->ops = &entryOps;

    en->flags = flags;
    en->cursorPosition = 0;
    en->firstChar = 0;
    en->bufUsed = 0;
    en->bufAlloced = width + 1;
    en->filter = NULL;

    if (!(en->flags & NEWT_FLAG_DISABLED))
	co->takesFocus = 1;
    else
	co->takesFocus = 0;

    if (initialValue && strlen(initialValue) > (unsigned int)width) {
	en->bufAlloced = strlen(initialValue) + 1;
    }
    en->buf = malloc(en->bufAlloced);
    en->resultPtr = resultPtr;
    if (en->resultPtr) *en->resultPtr = en->buf;

    memset(en->buf, 0, en->bufAlloced);
    if (initialValue) {
	strcpy(en->buf, initialValue);
	en->bufUsed = strlen(initialValue);
	en->cursorPosition = en->bufUsed;

	/* move cursor back if entry is full */
	if (en->cursorPosition && !(en->flags & NEWT_FLAG_SCROLL ||
		    wstrlen(en->buf, -1) < co->width))
	    en->cursorPosition = previous_char(en->buf, en->cursorPosition);
    } else {
	*en->buf = '\0';
	en->bufUsed = 0;
	en->cursorPosition = 0;
    }

    en->cs = NEWT_COLORSET_ENTRY;
    en->csDisabled = NEWT_COLORSET_DISENTRY;

    return co;
}

static void scroll(struct entry *en, int width)
{
    int r, lv, rv, cntx, cw, cn, nc, pc, ncw, pcw;

    if (width <= 1) {
	en->firstChar = en->cursorPosition;
	return;
    }

    cntx = width / 4;
    if (cntx > 5)
	cntx = 5;

    if (en->cursorPosition < en->firstChar)
	en->firstChar = en->cursorPosition;

    cn = next_char(en->buf, en->cursorPosition);
    cw = en->cursorPosition >= en->bufUsed ? 1 :
	wstrlen(en->buf + en->cursorPosition, cn - en->cursorPosition);

    r = wstrlen(en->buf + cn, -1);

    lv = wstrlen(en->buf + en->firstChar, en->cursorPosition - en->firstChar);
    rv = width - lv - cw;

#define RC (ncw > 0 && (r > rv && lv - ncw >= cntx && rv < cntx))
#define LC (pcw > 0 && (r + pcw <= rv || (lv < cntx && rv - pcw >= cntx)))

    nc = next_char(en->buf, en->firstChar);
    ncw = wstrlen(en->buf + en->firstChar, nc - en->firstChar);
    if (RC) {
	do {
	    lv -= ncw;
	    rv += ncw;
	    en->firstChar = nc;
	    nc = next_char(en->buf, en->firstChar);
	    ncw = wstrlen(en->buf + en->firstChar, nc - en->firstChar);
	} while (RC);
	return;
    }

    pc = previous_char(en->buf, en->firstChar);
    pcw = wstrlen(en->buf + pc, en->firstChar - pc);
    if (LC) {
	do {
	    lv += pcw;
	    rv -= pcw;
	    en->firstChar = pc;
	    pc = previous_char(en->buf, en->firstChar);
	    pcw = wstrlen(en->buf + pc, en->firstChar - pc);
	} while (LC);
    }
}

static void entryDraw(newtComponent co) {
    struct entry * en = co->data;
    int i;
    char * chptr;
    int len;
    char *tmpptr = NULL;

    if (!co->isMapped) return;

    if (en->flags & NEWT_FLAG_DISABLED)
	SLsmg_set_color(en->csDisabled);
    else
	SLsmg_set_color(en->cs);

    if (en->flags & NEWT_FLAG_HIDDEN) {
	newtGotorc(co->top, co->left);
	for (i = 0; i < co->width; i++)
	    SLsmg_write_char('_');
	newtGotorc(co->top, co->left);

	return;
    }

    newtTrashScreen();

    /* scroll if necessary */
    scroll(en, co->width);

    chptr = en->buf + en->firstChar;

    if (en->flags & NEWT_FLAG_PASSWORD) {
	len = wstrlen(chptr, -1);
	tmpptr = alloca(len + 1);
	for (i = 0; i < len; i++)
	    memset(tmpptr, '*', len);
	tmpptr[len] = '\0';
	chptr = tmpptr;
    }			

    len = wstrlen(chptr, -1);

    /* workaround for double width characters */
    if (co->width > 1) {
	i = len < co->width ? len : co->width;
	i = i > 2 ? i - 2 : 0;
	newtGotorc(co->top, co->left + i);
	SLsmg_write_char('_');
	SLsmg_write_char('_');
    }

    newtGotorc(co->top, co->left);

    if (len <= co->width) {
	i = len;
	SLsmg_write_string(chptr);
	while (i < co->width) {
	    SLsmg_write_char('_');
	    i++;
	}
    } else
	SLsmg_write_nstring(chptr, co->width);

    newtGotorc(co->top, co->left + wstrlen(en->buf+en->firstChar, en->cursorPosition - en->firstChar));
}

void newtEntrySetFlags(newtComponent co, int flags, enum newtFlagsSense sense) {
    struct entry * en = co->data;
    int row, col;

    en->flags = newtSetFlags(en->flags, flags, sense);

    if (!(en->flags & NEWT_FLAG_DISABLED))
	co->takesFocus = 1;
    else
	co->takesFocus = 0;

    newtGetrc(&row, &col);
    entryDraw(co);
    newtGotorc(row, col);
}

void newtEntrySetColors(newtComponent co, int normal, int disabled) {
    struct entry * en = co->data;

    en->cs = normal;
    en->csDisabled = disabled;
    entryDraw(co);
}

static void entryDestroy(newtComponent co) {
    struct entry * en = co->data;

    free(en->buf);
    free(en);
    free(co);
}

static struct eventResult entryEvent(newtComponent co,
				     struct event ev) {
    struct entry * en = co->data;
    struct eventResult er;
    int ch;

    er.result = ER_IGNORED;

    if (ev.when == EV_NORMAL) {
	switch (ev.event) {
	case EV_FOCUS:
	    newtCursorOn();
	    if (en->flags & NEWT_FLAG_HIDDEN)
		newtGotorc(co->top, co->left);
	    else
		newtGotorc(co->top, co->left +
			   wstrlen(en->buf + en->firstChar, en->cursorPosition - en->firstChar));
	    er.result = ER_SWALLOWED;
	    break;

	case EV_UNFOCUS:
	    newtCursorOff();
	    newtGotorc(0, 0);
	    er.result = ER_SWALLOWED;
	    if (co->callback)
		co->callback(co, co->callbackData);
	    break;

	case EV_KEYPRESS:
	    ch = ev.u.key;
	    if (en->filter)
		ch = en->filter(co, en->filterData, ch, en->cursorPosition);
	    if (ch) er = entryHandleKey(co, ch);
	    break;

	case EV_MOUSE:
	    if ((ev.u.mouse.type == MOUSE_BUTTON_DOWN) &&
		(en->flags ^ NEWT_FLAG_HIDDEN)) {
		if (strlen(en->buf) >= ev.u.mouse.x - co->left) {
		    en->cursorPosition = ev.u.mouse.x - co->left;
		    newtGotorc(co->top,
			       co->left +(en->cursorPosition - en->firstChar));
		} else {
		    en->cursorPosition = strlen(en->buf);
		    newtGotorc(co->top,
			       co->left +(en->cursorPosition - en->firstChar));
		}
	    }
	    break;
	}
    }

    return er;
}

static int previous_char(const char *buf, int pos)
{
    int len = 0;
    int off = 0;
    
    while (off < pos) {
       len = mblen(buf+off, MB_CUR_MAX);
       if (len <= 0)
	  return pos;
       off+=len;
    }
    return off-len;
}

static int next_char(const char *buf, int pos)
{
    int len = mblen(buf + pos, MB_CUR_MAX);
    if (len <= 0)
       return pos;
    return pos+len;
}

static struct eventResult entryHandleKey(newtComponent co, int key) {
    struct entry * en = co->data;
    struct eventResult er;
    char * chptr;

    er.result = ER_SWALLOWED;
    switch (key) {
      case '\r':				/* Return */
	if (en->flags & NEWT_FLAG_RETURNEXIT) {
	    newtCursorOff();
	    er.result = ER_EXITFORM;
	} else {
	    er.result = ER_NEXTCOMP;
	}
	break;

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

      case '\025':				/* ^U */
	en->bufUsed -= en->cursorPosition;
	memmove(en->buf, en->buf + en->cursorPosition, en->bufUsed);
	en->cursorPosition = 0;
	memset(en->buf + en->bufUsed, 0, en->bufAlloced - en->bufUsed);
	break;

      case '\002':				/* ^B */
      case NEWT_KEY_LEFT:
	if (en->cursorPosition)
	    en->cursorPosition = previous_char(en->buf, en->cursorPosition);
	break;

      case '\004':
      case NEWT_KEY_DELETE:
	chptr = en->buf + en->cursorPosition;
	if (*chptr) {
	    int delta = next_char(en->buf, en->cursorPosition)-en->cursorPosition;
	    if (delta) {
	       chptr+=delta;
	       while (*chptr) {
	          *(chptr - delta) = *chptr;
		  chptr++;
	       }
	       memset(chptr - delta, 0, delta);
	       en->bufUsed-=delta;
	    }
	}
	break;

      case NEWT_KEY_BKSPC: {
	int prev = previous_char(en->buf, en->cursorPosition);
	if (en->cursorPosition != prev) {
	    /* if this isn't true, there's nothing to erase */
	    int delta = en->cursorPosition - prev;
	    chptr = en->buf + en->cursorPosition;
	    en->bufUsed-=delta;
	    en->cursorPosition-=delta;
	    while (*chptr) {
		*(chptr - delta) = *chptr;
		chptr++;
	    }
	    memset(chptr - delta, 0, delta);
	}
	}
	break;

      case '\006':				/* ^B */
      case NEWT_KEY_RIGHT:
	if (en->cursorPosition < en->bufUsed)
	    en->cursorPosition = next_char(en->buf, en->cursorPosition);
	break;

      default:
	if ((key >= 0x20 && key <= 0x7e) || (key >= 0x80 && key <= 0xff)) {
	    char s[MB_CUR_MAX];
	    mbstate_t ps;
	    int i, l;

	    for (i = 1, s[0] = key; ; i++) {
		memset(&ps, 0, sizeof (ps));
		l = mbrtowc(NULL, s, i, &ps);
		if (l == -1)	/* invalid sequence */
		    i = 0;
		if (l != -2)	/* not incomplete sequence */
		    break;

		/* read next byte */
		if (i == MB_CUR_MAX || !SLang_input_pending(1)) {
		    i = 0;
		    break;
		}
		s[i] = SLang_getkey();
	    }

	    if (!i || (!(en->flags & NEWT_FLAG_SCROLL) && wstrlen(en->buf, -1) + wstrlen(s, i) > co->width)) {
		/* FIXME this is broken */
		SLtt_beep();
		break;
	    }

	    if ((en->bufUsed + i) >= en->bufAlloced) {
		en->bufAlloced += 20;
		en->buf = realloc(en->buf, en->bufAlloced);
		if (en->resultPtr) *en->resultPtr = en->buf;
		memset(en->buf + en->bufAlloced - 20, 0, 20);
	    }

	    if (en->cursorPosition != en->bufUsed) {
		/* insert the new character */
		memmove(en->buf + en->cursorPosition + i, en->buf + en->cursorPosition, en->bufUsed - en->cursorPosition);
	    }
	    en->bufUsed += i;
	    for (l = 0; l < i; l++)
		en->buf[en->cursorPosition++] = s[l];
	} else {
	    er.result = ER_IGNORED;
	}
    }

    if (en->cursorPosition == en->bufUsed && en->cursorPosition &&
	    !(en->flags & NEWT_FLAG_SCROLL || wstrlen(en->buf, -1) < co->width))
	en->cursorPosition = previous_char(en->buf, en->cursorPosition);

    entryDraw(co);

    return er;
}

char * newtEntryGetValue(newtComponent co) {
    struct entry * en = co->data;

    return en->buf;
}

void newtEntrySetFilter(newtComponent co, newtEntryFilter filter, void * data) {
    struct entry * en = co->data;
    en->filter = filter;
    en->filterData = data;
}

int newtEntryGetCursorPosition (newtComponent co) {
    struct entry * en = co->data;

    return en->cursorPosition;
}

void newtEntrySetCursorPosition (newtComponent co, int position) {
    struct entry * en = co->data;

    en->cursorPosition = position;
}
