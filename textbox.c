#include <ctype.h>
#include <slang/slang.h>
#include <stdlib.h>
#include <string.h>

#include "newt.h"
#include "newt_pr.h"

struct textbox {
    char ** lines;
    int numLines;
    int linesAlloced;
    int doWrap;
    newtComponent sb;
    int topLine;
};

static void addLine(newtComponent co, const char * s, int len);
static void textboxDraw(newtComponent co);
static void addShortLine(newtComponent co, const char * s, int len);
static struct eventResult textboxEvent(newtComponent c,
				      struct event ev);
static void textboxDestroy(newtComponent co);

static struct componentOps textboxOps = {
    textboxDraw,
    textboxEvent,
    textboxDestroy,
    NULL,
} ;

void newtTextboxSetHeight(newtComponent co, int height) {
    co->height = height;
}

int newtTextboxGetNumLines(newtComponent co) {
    struct textbox * tb = co->data;

    return (tb->numLines);
}

newtComponent newtTextbox(int left, int top, int width, int height, int flags) {
    newtComponent co;
    struct textbox * tb;

    co = malloc(sizeof(*co));
    tb = malloc(sizeof(*tb));
    co->data = tb;

    co->ops = &textboxOps;

    co->height = height;
    co->top = top;
    co->left = left;
    co->takesFocus = 0;

    tb->doWrap = flags & NEWT_FLAG_WRAP;
    tb->numLines = 0;
    tb->linesAlloced = 0;
    tb->lines = NULL;
    tb->topLine = 0;

    if (flags & NEWT_FLAG_SCROLL) {
	co->width = width - 2;
	tb->sb = newtVerticalScrollbar(co->left + co->width + 1, co->top, 
			   co->height, COLORSET_TEXTBOX, COLORSET_TEXTBOX);
    } else {
	co->width = width;
	tb->sb = NULL;
    }

    return co;
}

static void doReflow(char * text, char ** resultPtr, int width, int * badness,
		     int * heightPtr) {
    char * result = NULL;
    char * chptr, * end;
    int howbad = 0;
    int height = 0;

    if (resultPtr) {
	/* XXX I think this will work */
	result = malloc(strlen(text) + (strlen(text) / width) + 2);
	*result = '\0';
    }
    
    while (*text) {
	end = strchr(text, '\n');
	if (!end)
	    end = text + strlen(text);

	while (*text && text <= end) {
	    if (end - text < width) {
		if (result) {
		    strncat(result, text, end - text);
		    strcat(result, "\n");
		    height++;
		}

		if (end - text < (width / 2))
		    howbad += ((width / 2) - (end - text)) / 2;
		text = end;
		if (*text) text++;
	    } else {
		chptr = text + width - 1;
		while (chptr > text && !isspace(*chptr)) chptr--;
		while (isspace(*chptr)) chptr--;
		chptr++;

		if (chptr > text)
		    howbad += width - (chptr - text) + 1;
		if (result) {
		    strncat(result, text, chptr - text);
		    strcat(result, "\n");
		    height++;
		}

		text = chptr + 1;
		while (isspace(*text)) text++;
	    }
	}
    }

    if (badness) *badness = howbad;
    if (resultPtr) *resultPtr = result;
    if (heightPtr) *heightPtr = height;
}

char * newtReflowText(char * text, int width, int flexDown, int flexUp,
		      int * actualWidth, int * actualHeight) {
    int min, max;
    int i;
    char * result;
    int minbad, minbadwidth, howbad;

    if (flexDown || flexUp) {
	min = width - flexDown;
	max = width + flexUp;

	minbad = -1;
	minbadwidth = width;

	for (i = min; i <= max; i++) {
	    doReflow(text, NULL, i, &howbad, NULL);

	    if (minbad == -1 || howbad < minbad) {
		minbad = howbad;
		minbadwidth = i;
	    }
 	}

	width = minbadwidth;
    }

    doReflow(text, &result, width, NULL, actualHeight);
    if (actualWidth) *actualWidth = width;
    return result;
}

void newtTextboxSetText(newtComponent co, const char * text) {
    const char * start, * end;
    struct textbox * tb = co->data;

    if (tb->lines) {
	free(tb->lines);
	tb->numLines = 0;
    }

    tb->linesAlloced = 10;
    tb->lines = malloc(sizeof(char *) * 10);

    start = text;
    while ((end = strchr(start, '\n'))) {
	addLine(co, start, end - start);
	start = end + 1;
    }

    if (*start)
	addLine(co, start, -1);
}

static void addLine(newtComponent co, const char * s, int origlen) {
    const char * start, * end;
    int len;
    struct textbox * tb = co->data;

    if (origlen < 0) origlen = strlen(s);
    len = origlen;

    if (!tb->doWrap || len <= co->width) {
	addShortLine(co, s, len);
    } else {
	/* word wrap */

	start = s;
	while (len > co->width) {
	    end = start + co->width - 1;
	    while (end > start && !isspace(*end)) end--;

	    if (end == start)
		end = start + co->width - 1;

	    addShortLine(co, start, end - start);
	
	    start = end + 1;
	    while (isspace(*start) && *start) start++;

	    len = origlen - (start - s);
	}	

	if (*start)
	    addShortLine(co, start, len);
    }
}

static void addShortLine(newtComponent co, const char * s, int len) {
    struct textbox * tb = co->data;

    if (tb->linesAlloced == tb->numLines) {
	tb->linesAlloced += 10;
	tb->lines = realloc(tb->lines, (sizeof(char *) * tb->linesAlloced));
    }

    if (len > co->width) len = co->width;

    tb->lines[tb->numLines] = malloc(co->width + 1);
    strncpy(tb->lines[tb->numLines], s, len);

    while (len < co->width)  tb->lines[tb->numLines][len++] = ' ';
    tb->lines[tb->numLines++][len] = '\0';
}

static void textboxDraw(newtComponent c) {
    int i;
    struct textbox * tb = c->data;
    int size;

    if (tb->sb) {
	size = tb->numLines - c->height;
	newtScrollbarSet(tb->sb, tb->topLine, size ? size : 0);
	tb->sb->ops->draw(tb->sb);
    }

    SLsmg_set_color(NEWT_COLORSET_TEXTBOX);
   
    for (i = 0; (i + tb->topLine) < tb->numLines && i < c->height; i++) {
	newtGotorc(c->top + i, c->left);
	SLsmg_write_string(tb->lines[i + tb->topLine]);
    }
}

static struct eventResult textboxEvent(newtComponent co, 
				      struct event ev) {
    struct textbox * tb = co->data;
    struct eventResult er;

    er.result = ER_IGNORED;

    if (ev.when == EV_EARLY && ev.event == EV_KEYPRESS && tb->sb) {
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
	}
    }

    return er;
}

static void textboxDestroy(newtComponent co) {
    int i;
    struct textbox * tb = co->data;

    for (i = 0; i < tb->numLines; i++) 
	free(tb->lines[i]);
    free(tb->lines);
    free(tb);
    free(co);
}
