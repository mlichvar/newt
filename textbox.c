
#include <ctype.h>
#include <slang.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#include "newt.h"
#include "newt_pr.h"

struct textbox {
    char ** lines;
    int numLines;
    int linesAlloced;
    int doWrap;
    newtComponent sb;
    int topLine;
    int textWidth;
    int isActive;
    int cs;
    int csActive;
};

static char * expandTabs(const char * text);
static void textboxDraw(newtComponent co);
static void addLine(newtComponent co, const char * s, int len);
static void doReflow(const char * text, char ** resultPtr, int width, 
		     int * badness, int * heightPtr);
static struct eventResult textboxEvent(newtComponent c,
				      struct event ev);
static void textboxDestroy(newtComponent co);
static void textboxPlace(newtComponent co, int newLeft, int newTop);
static void textboxMapped(newtComponent co, int isMapped);

static struct componentOps textboxOps = {
    textboxDraw,
    textboxEvent,
    textboxDestroy,
    textboxPlace,
    textboxMapped,
} ;

static void textboxMapped(newtComponent co, int isMapped) {
    struct textbox * tb = co->data;

    co->isMapped = isMapped;
    if (tb->sb) {
	tb->sb->ops->mapped(tb->sb, isMapped);
    }
}

static void textboxPlace(newtComponent co, int newLeft, int newTop) {
    struct textbox * tb = co->data;

    co->top = newTop;
    co->left = newLeft;

    if (tb->sb) {
	tb->sb->ops->place(tb->sb, co->left + co->width - 1, co->top);
    }
}

void newtTextboxSetHeight(newtComponent co, int height) {
    co->height = height;
}

int newtTextboxGetNumLines(newtComponent co) {
    struct textbox * tb = co->data;

    return (tb->numLines);
}

newtComponent newtTextboxReflowed(int left, int top, char * text, int width,
				  int flexDown, int flexUp, int flags) {
    newtComponent co;
    char * reflowedText;
    int actWidth, actHeight;

    reflowedText = newtReflowText(text, width, flexDown, flexUp,
				  &actWidth, &actHeight);
    
    co = newtTextbox(left, top, actWidth, actHeight, NEWT_FLAG_WRAP);
    newtTextboxSetText(co, reflowedText);
    free(reflowedText);

    return co;
}

newtComponent newtTextbox(int left, int top, int width, int height, int flags) {
    newtComponent co;
    struct textbox * tb;

    co = malloc(sizeof(*co));
    tb = malloc(sizeof(*tb));
    co->data = tb;

    if (width < 1)
	width = 1;

    co->ops = &textboxOps;

    co->isMapped = 0;
    co->height = height;
    co->top = top;
    co->left = left;
    co->takesFocus = 0;
    co->width = width;
    co->destroyCallback = NULL;

    tb->doWrap = flags & NEWT_FLAG_WRAP;
    tb->numLines = 0;
    tb->linesAlloced = 0;
    tb->lines = NULL;
    tb->topLine = 0;
    tb->textWidth = width;
    tb->isActive = 0;
    tb->cs = COLORSET_TEXTBOX;
    tb->csActive = COLORSET_ACTTEXTBOX;

    if (flags & NEWT_FLAG_SCROLL) {
	co->width += 2;
	tb->sb = newtVerticalScrollbar(co->left + co->width - 1, co->top, 
			   co->height, tb->cs, tb->cs);
	co->takesFocus = 1;
    } else {
	tb->sb = NULL;
    }

    return co;
}

void newtTextboxSetColors(newtComponent co, int normal, int active) {
    struct textbox * tb = co->data;

    tb->cs = normal;
    tb->csActive = active;
    textboxDraw(co);
}

static char * expandTabs(const char * text) {
    int bufAlloced = strlen(text) + 40;
    char * buf, * dest;
    const char * src;
    int bufUsed = 0;
    int linePos = 0;
    int i;

    buf = malloc(bufAlloced + 1);
    for (src = text, dest = buf; *src; src++) {
	if ((bufUsed + 10) > bufAlloced) {
	    bufAlloced += strlen(text) / 2;
	    buf = realloc(buf, bufAlloced + 1);
	    dest = buf + bufUsed;
	}
	if (*src == '\t') {
	    i = 8 - (linePos & 8);
	    memset(dest, ' ', i);
	    dest += i, bufUsed += i, linePos += i;
	} else {
	    if (*src == '\n')
		linePos = 0;
	    else
		linePos++;

	    *dest++ = *src;
	    bufUsed++;
	}
    }

    *dest = '\0';
    return buf;
}

static void doReflow(const char * text, char ** resultPtr, int width, 
		     int * badness, int * heightPtr) {
    char * result = NULL;
    const char * chptr, * end;
    int i;
    int howbad = 0;
    int height = 0;
    wchar_t tmp;
    mbstate_t ps;

    if (resultPtr) {
	if (width > 1) {
	    /* use width - 1 for double width characters
	       that won't fit at end of line */
	    result = malloc(strlen(text) + (strlen(text) / (width - 1)) + 2);
	} else
	    result = malloc(strlen(text) * 2 + 2);
	*resultPtr = result;
    }
	
    memset(&ps,0,sizeof(mbstate_t));
    while (*text) {
	end = strchr(text, '\n');
	if (!end)
	    end = text + strlen(text);

	while (*text && text <= end) {
	    int len;
		
	    len = wstrlen(text, end - text);
	    if (len <= width) {
		if (result) {
		    memcpy(result, text, end - text);
		    result += end - text;
		    *result++ = '\n';
		    height++;
		}

		if (len < (width / 2)) {
#ifdef DEBUG_WRAP		    
		fprintf(stderr,"adding %d\n",((width / 2) - (len)) / 2);
#endif					
		    howbad += ((width / 2) - (len)) / 2;
		}
		text = end;
		if (*text) text++;
	    } else {
		const char *spcptr = NULL;
	        int spc = 0, w2 = 0, x, w;

	        chptr = text;
		i = 0;
		while (1) {
			if ((x=mbrtowc(&tmp,chptr,end-chptr,&ps))<=0)
				break;
		        if (spc && !iswspace(tmp))
				spc = 0;
			else if (!spc && iswspace(tmp)) {
				spc = 1;
				spcptr = chptr;
				w2 = i;
			}
			w = wcwidth(tmp);
			if (w < 0)
				w = 0;
			if (i && w + i > width)
				break;
			chptr += x;
			i += w;
		}
		howbad += width - w2 + 1;
#ifdef DEBUG_WRAP		    
		fprintf(stderr,"adding %d\n",width - w2 + 1, chptr);
#endif					
		if (spcptr) chptr = spcptr;
		if (result) {
		    memcpy(result, text, chptr - text);
		    result += chptr - text;
		    *result++ = '\n';
		    height++;
		}

		text = chptr;
		while (1) {
			if ((x=mbrtowc(&tmp,text,end-text,NULL))<=0)
				break;
			if (!iswspace(tmp)) break;
			text += x;
		}
	    }
	}
    }

    if (result)
	*result = '\0';

    if (badness) *badness = howbad;
    if (heightPtr) *heightPtr = height;
#ifdef DEBUG_WRAP
    fprintf(stderr, "width %d, badness %d, height %d\n",width, howbad, height);
#endif
}

char * newtReflowText(char * text, int width, int flexDown, int flexUp,
		      int * actualWidth, int * actualHeight) {
    int min, max;
    int i;
    char * result;
    int minbad, minbadwidth, howbad;
    char * expandedText;

    if (width < 1)
	width = 1;

    expandedText = expandTabs(text);

    if (flexDown || flexUp) {
	min = width - flexDown;
	max = width + flexUp;

	minbad = -1;
	minbadwidth = width;

	for (i = min; i >= 1 && i <= max; i++) {
	    doReflow(expandedText, NULL, i, &howbad, NULL);

	    if (minbad == -1 || howbad < minbad) {
		minbad = howbad;
		minbadwidth = i;
	    }
 	}

	width = minbadwidth;
    }

    doReflow(expandedText, &result, width, NULL, actualHeight);
    free(expandedText);
    if (actualWidth) *actualWidth = width;
    return result;
}

void newtTextboxSetText(newtComponent co, const char * text) {
    const char * start, * end;
    struct textbox * tb = co->data;
    char * reflowed, * expanded;
    int badness, height;

    if (tb->lines) {
	int i;

	for (i = 0; i < tb->numLines; i++) 
	    free(tb->lines[i]);
	free(tb->lines);
	tb->linesAlloced = tb->numLines = tb->topLine = 0;
    }

    expanded = expandTabs(text);

    if (tb->doWrap) {
	doReflow(expanded, &reflowed, tb->textWidth, &badness, &height);
	free(expanded);
	expanded = reflowed;
    }

    for (start = expanded; *start; start++)
	if (*start == '\n') tb->linesAlloced++;

    /* This ++ leaves room for an ending line w/o a \n */
    tb->linesAlloced++;
    tb->lines = malloc(sizeof(char *) * tb->linesAlloced);

    start = expanded;
    while ((end = strchr(start, '\n'))) {
	addLine(co, start, end - start);
	start = end + 1;
    }

    if (*start)
	addLine(co, start, strlen(start));

    free(expanded);
    
    textboxDraw(co);

    newtTrashScreen();    
}

/* This assumes the buffer is allocated properly! */
static void addLine(newtComponent co, const char * s, int len) {
    struct textbox * tb = co->data;

    while (wstrlen(s,len) > tb->textWidth) {
	    len--;
    }
    tb->lines[tb->numLines] = malloc(len + 1);
    memcpy(tb->lines[tb->numLines], s, len);
    tb->lines[tb->numLines++][len] = '\0';
}

static void textboxDraw(newtComponent c) {
    int i;
    struct textbox * tb = c->data;
    int size;

    if (!c->isMapped)
	    return;

    if (tb->sb) {
	size = tb->numLines - c->height;
	newtScrollbarSet(tb->sb, tb->topLine, size ? size : 0);
	newtScrollbarSetColors(tb->sb, tb->isActive ? tb->csActive :
			tb->cs, tb->cs);
    }

    SLsmg_set_color(tb->cs);

    for (i = 0; (i + tb->topLine) < tb->numLines && i < c->height; i++) {
	newtGotorc(c->top + i, c->left);
	SLsmg_write_nstring(tb->lines[i + tb->topLine], tb->textWidth);
    }
    /* put cursor at beginning of text for better accessibility */
    newtGotorc(c->top, c->left);
}

static struct eventResult textboxEvent(newtComponent co, 
				      struct event ev) {
    struct textbox * tb = co->data;
    struct eventResult er;

    er.result = ER_IGNORED;

    if (!tb->sb || ev.when == EV_EARLY || ev.when == EV_LATE)
	return er;

    switch(ev.event) {
      case EV_KEYPRESS:
	newtTrashScreen();
	switch (ev.u.key) {
	  case NEWT_KEY_UP:
	    if (tb->topLine) tb->topLine--;
	    textboxDraw(co);
	    er.result = ER_SWALLOWED;
	    break;

	  case NEWT_KEY_DOWN:
	    if (tb->topLine < (tb->numLines - co->height)) tb->topLine++;
	    textboxDraw(co);
	    er.result = ER_SWALLOWED;
	    break;

	  case NEWT_KEY_PGDN:
	    tb->topLine += co->height;
	    if (tb->topLine > (tb->numLines - co->height)) {
		tb->topLine = tb->numLines - co->height;
		if (tb->topLine < 0) tb->topLine = 0;
	    }
	    textboxDraw(co);
	    er.result = ER_SWALLOWED;
	    break;

	  case NEWT_KEY_PGUP:
	    tb->topLine -= co->height;
	    if (tb->topLine < 0) tb->topLine = 0;
	    textboxDraw(co);
	    er.result = ER_SWALLOWED;
	    break;
	}
	break;
      case EV_MOUSE:
	/* Top scroll arrow */
	if (ev.u.mouse.x == co->width && ev.u.mouse.y == co->top) {
	    if (tb->topLine) tb->topLine--;
	    textboxDraw(co);
	    
	    er.result = ER_SWALLOWED;
	}
	/* Bottom scroll arrow */
	if (ev.u.mouse.x == co->width &&
	    ev.u.mouse.y == co->top + co->height - 1) {
	    if (tb->topLine < (tb->numLines - co->height)) tb->topLine++;
	    textboxDraw(co);
	    
	    er.result = ER_SWALLOWED;
	}
	break;
      case EV_FOCUS:
	tb->isActive = 1;
	textboxDraw(co);
	er.result = ER_SWALLOWED;
	break;
      case EV_UNFOCUS:
	tb->isActive = 0;
	textboxDraw(co);
	er.result = ER_SWALLOWED;
	break;
    }
    return er;
}

static void textboxDestroy(newtComponent co) {
    int i;
    struct textbox * tb = co->data;

    if (tb->sb)
	tb->sb->ops->destroy(tb->sb);
    for (i = 0; i < tb->numLines; i++) 
	free(tb->lines[i]);
    free(tb->lines);
    free(tb);
    free(co);
}
