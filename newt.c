#include <slang/slang.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

#include "newt.h"
#include "newt_pr.h"

struct Window {
    int height, width, top, left;
    short * buffer;
};

struct keymap {
    char * str;
    int code;
    char * tc;
};

static struct Window windowStack[20];
static struct Window * currentWindow = NULL;

static char * helplineStack[20];
static char ** currentHelpline = NULL;

static int cursorRow, cursorCol;

static char * defaultHelpLine = 
"  <Tab>/<Alt-Tab> between elements   |  <Space> selects   |  <F12> next screen"
;

struct newtColors newtDefaultColorPalette = {
	"white", "blue", 			/* root fg, bg */     
	"black", "lightgray",			/* border fg, bg */
	"black", "lightgray",			/* window fg, bg */
	"white", "black",			/* shadow fg, bg */
	"red", "lightgray",			/* title fg, bg */
	"lightgray", "red",			/* button fg, bg */
	"red", "lightgray",			/* active button fg, bg */
	"yellow", "blue",			/* checkbox fg, bg */
	"blue", "brown",			/* active checkbox fg, bg */
	"yellow", "blue",			/* entry box fg, bg */
	"blue", "lightgray",			/* label fg, bg */
	"black", "lightgray",			/* listbox fg, bg */
	"yellow", "blue",			/* active listbox fg, bg */
	"black", "lightgray",			/* textbox fg, bg */
	"lightgray", "black",			/* active textbox fg, bg */
	"white", "blue",			/* help line */
	"yellow", "blue",			/* root text */
	"blue",					/* scale full */
	"red",					/* scale empty */
	"blue", "lightgray",			/* disabled entry fg, bg */
};

static struct keymap keymap[] = {
	{ "\033OA", 		NEWT_KEY_UP, 		"kh" },
	{ "\033[A", 		NEWT_KEY_UP, 		"ku" },
	{ "\033OB", 		NEWT_KEY_DOWN, 		"kd" },
	{ "\033[B", 		NEWT_KEY_DOWN, 		"kd" },
	{ "\033[C", 		NEWT_KEY_RIGHT, 	"kr" },
	{ "\033OC", 		NEWT_KEY_RIGHT, 	"kr" },
	{ "\033[D", 		NEWT_KEY_LEFT, 		"kl" },
	{ "\033OD", 		NEWT_KEY_LEFT, 		"kl" },
	{ "\033[H",		NEWT_KEY_HOME, 		"kh" },
	{ "\033[1~",		NEWT_KEY_HOME, 		"kh" },
	{ "\033Ow",		NEWT_KEY_END, 		"kH" },
	{ "\033[4~",		NEWT_KEY_END, 		"kH" },

	{ "\033[3~",		NEWT_KEY_DELETE,	"kl" },

	{ "\033\t",		NEWT_KEY_UNTAB,		NULL },

	{ "\033[5~",		NEWT_KEY_PGUP,		NULL },
	{ "\033[6~",		NEWT_KEY_PGDN,		NULL },

	{ "\033[[A",		NEWT_KEY_F1,		NULL },
	{ "\033[[B",		NEWT_KEY_F2,		NULL },
	{ "\033[[C",		NEWT_KEY_F3,		NULL },
	{ "\033[[D",		NEWT_KEY_F4,		NULL },
	{ "\033[[E",		NEWT_KEY_F5,		NULL },

	{ "\033[11~",		NEWT_KEY_F1,		NULL },
	{ "\033[12~",		NEWT_KEY_F2,		NULL },
	{ "\033[13~",		NEWT_KEY_F3,		NULL },
	{ "\033[14~",		NEWT_KEY_F4,		NULL },
	{ "\033[15~",		NEWT_KEY_F5,		NULL },
	{ "\033[17~",		NEWT_KEY_F6,		NULL },
	{ "\033[18~",		NEWT_KEY_F7,		NULL },
	{ "\033[19~",		NEWT_KEY_F8,		NULL },
	{ "\033[20~",		NEWT_KEY_F9,		NULL },
	{ "\033[21~",		NEWT_KEY_F10,		NULL },
	{ "\033[23~",		NEWT_KEY_F11,		NULL },
	{ "\033[24~",		NEWT_KEY_F12,		NULL },

	{ NULL, 	0, 			NULL },	/* LEAVE this one */
};
static char keyPrefix = '\033';

static char * version = "Newt windowing library version " VERSION
			" - (C) 1996 Red Hat Software. "
		        "Redistributable under the term of the Library "
		        "GNU Public Library. "
			"Written by Erik Troan\n";

void newtRefresh(void) {
    SLsmg_refresh();
}

void newtSuspend(void) {
    SLsmg_suspend_smg();
    SLang_reset_tty();
}

void newtResume(void) {
    SLsmg_resume_smg ();
    SLsmg_refresh();
    SLang_init_tty(0, 0, 0);
}

void newtCls(void) {
    SLsmg_set_color(NEWT_COLORSET_ROOT);
    SLsmg_gotorc(0, 0);
    SLsmg_erase_eos();

    newtRefresh();
}

int newtInit(void) {
    struct winsize ws;
    char * MonoValue, * MonoEnv = "NEWT_MONO";

    /* use the version variable just to be sure it gets included */
    strlen(version);

    SLtt_get_terminfo();

    if (!ioctl(1, TIOCGWINSZ, &ws)) {
	SLtt_Screen_Rows = ws.ws_row;
	SLtt_Screen_Cols = ws.ws_col;
    }

    MonoValue = getenv(MonoEnv);
    if ( MonoValue == NULL ) {
	SLtt_Use_Ansi_Colors = 1;
    } else {
	SLtt_Use_Ansi_Colors = 0;
    }

    SLsmg_init_smg();
    SLang_init_tty(0, 0, 0);
 
    newtSetColors(newtDefaultColorPalette);
    /*initKeymap();*/

    /*SLtt_set_cursor_visibility(0);*/

    return 0;
}

int newtFinished(void) {
    SLsmg_gotorc(SLtt_Screen_Rows - 1, 0);
    SLsmg_refresh();
    SLsmg_reset_smg();
    SLang_reset_tty();

    return 0;
}

void newtSetColors(struct newtColors colors) {
    SLtt_set_color(NEWT_COLORSET_ROOT, "", colors.rootFg, colors.rootBg);
    SLtt_set_color(NEWT_COLORSET_BORDER, "", colors.borderFg, colors.borderBg);
    SLtt_set_color(NEWT_COLORSET_WINDOW, "", colors.windowFg, colors.windowBg);
    SLtt_set_color(NEWT_COLORSET_SHADOW, "", colors.shadowFg, colors.shadowBg);
    SLtt_set_color(NEWT_COLORSET_TITLE, "", colors.titleFg, colors.titleBg);
    SLtt_set_color(NEWT_COLORSET_BUTTON, "", colors.buttonFg, colors.buttonBg);
    SLtt_set_color(NEWT_COLORSET_ACTBUTTON, "", colors.actButtonFg, 
			colors.actButtonBg);
    SLtt_set_color(NEWT_COLORSET_CHECKBOX, "", colors.checkboxFg, 
			colors.checkboxBg);
    SLtt_set_color(NEWT_COLORSET_ACTCHECKBOX, "", colors.actCheckboxFg, 
			colors.actCheckboxBg);
    SLtt_set_color(NEWT_COLORSET_ENTRY, "", colors.entryFg, colors.entryBg);
    SLtt_set_color(NEWT_COLORSET_LABEL, "", colors.labelFg, colors.labelBg);
    SLtt_set_color(NEWT_COLORSET_LISTBOX, "", colors.listboxFg, 
			colors.listboxBg);
    SLtt_set_color(NEWT_COLORSET_ACTLISTBOX, "", colors.actListboxFg, 
			colors.actListboxBg);
    SLtt_set_color(NEWT_COLORSET_TEXTBOX, "", colors.textboxFg, 
			colors.textboxBg);
    SLtt_set_color(NEWT_COLORSET_ACTTEXTBOX, "", colors.actTextboxFg, 
			colors.actTextboxBg);
    SLtt_set_color(NEWT_COLORSET_HELPLINE, "", colors.helpLineFg, 
			colors.helpLineBg);
    SLtt_set_color(NEWT_COLORSET_ROOTTEXT, "", colors.rootTextFg, 
			colors.rootTextBg);

    SLtt_set_color(NEWT_COLORSET_EMPTYSCALE, "", "black",
			colors.emptyScale);
    SLtt_set_color(NEWT_COLORSET_FULLSCALE, "", "black",
			colors.fullScale);
    SLtt_set_color(NEWT_COLORSET_DISENTRY, "", colors.disabledEntryFg,
			colors.disabledEntryBg);
}

int newtGetKey(void) {
    int key;
    char buf[10], * chptr = buf;
    struct keymap * curr;

    key = SLang_getkey();

    switch (key) {
      case 0x7f:
	return NEWT_KEY_BKSPC;

      case 0x08:
	return NEWT_KEY_BKSPC;

      default:
	if (key != keyPrefix) return key;
    }

    memset(buf, 0, sizeof(buf));

    *chptr++ = key;
    while (SLang_input_pending(5)) {
	key = SLang_getkey();
	if (key == keyPrefix) {
	    /* he hit unknown keys too many times -- start over */
	    memset(buf, 0, sizeof(buf));
	    chptr = buf;
	}

	*chptr++ = key;

	/* this search should use bsearch(), but when we only look through
	   a list of 20 (or so) keymappings, it's probably faster just to
	   do a inline linear search */

	for (curr = keymap; curr->code; curr++) {
	    if (curr->str) {
		if (!strcmp(curr->str, buf))
		    return curr->code;
	    }
	}
    }

    for (curr = keymap; curr->code; curr++) {
	if (curr->str) {
	    if (!strcmp(curr->str, buf))
		return curr->code;
	}
    }

    /* Looks like we were a bit overzealous in reading characters. Return
       just the first character, and put everything else back in the buffer
       for later */

    chptr--;
    while (chptr > buf) 
	SLang_ungetkey(*chptr--);

    return *chptr;
}

void newtWaitForKey(void) {
    newtRefresh();

    SLang_getkey();
    newtClearKeyBuffer();
}

void newtClearKeyBuffer(void) {
    while (SLang_input_pending(1)) {
	SLang_getkey();
    }
}

int newtOpenWindow(int left, int top, int width, int height, 
			  char * title) {
    int i, j, row, col;
    int n;

    if (!currentWindow) {
	currentWindow = windowStack;
    } else {
	currentWindow++;
    }

    currentWindow->left = left;
    currentWindow->top = top;
    currentWindow->width = width;
    currentWindow->height = height;

    currentWindow->buffer = malloc(sizeof(short) * (width + 3) * (height + 3));

    row = top - 1;
    col = left - 1;
    n = 0;
    for (j = 0; j < height + 3; j++, row++) {
	SLsmg_gotorc(row, col);
	SLsmg_read_raw(currentWindow->buffer + n,
				currentWindow->width + 3);
	n += currentWindow->width + 3;
    }

    SLsmg_set_color(NEWT_COLORSET_BORDER);
    SLsmg_draw_box(top - 1, left - 1, height + 2, width + 2);

    if (title) {
	i = strlen(title) + 4;
	i = ((width - i) / 2) + left;
	SLsmg_gotorc(top - 1, i);
	SLsmg_set_char_set(1);
	SLsmg_write_char(SLSMG_RTEE_CHAR);
	SLsmg_set_char_set(0);
	SLsmg_write_char(' ');
	SLsmg_set_color(NEWT_COLORSET_TITLE);
	SLsmg_write_string(title);
	SLsmg_set_color(NEWT_COLORSET_BORDER);
	SLsmg_write_char(' ');
	SLsmg_set_char_set(1);
	SLsmg_write_char(SLSMG_LTEE_CHAR);
	SLsmg_set_char_set(0);
    }

    SLsmg_set_color(NEWT_COLORSET_WINDOW);
    SLsmg_fill_region(top, left, height, width, ' ');

    SLsmg_set_color(NEWT_COLORSET_SHADOW);
    SLsmg_fill_region(top + height + 1, left, 1, width + 2, ' ');
    SLsmg_fill_region(top, left + width + 1, height + 1, 1, ' ');

    for (i = top; i < (top + height + 1); i++) {
	SLsmg_gotorc(i, left + width + 1);
	SLsmg_write_string(" ");
    }

    return 0;
}

void newtPopWindow(void) {
    int j, row, col;
    int n = 0;    

    row = col = 0;

    row = currentWindow->top - 1;
    col = currentWindow->left - 1;
    for (j = 0; j < currentWindow->height + 3; j++, row++) {
	SLsmg_gotorc(row, col);
	SLsmg_write_raw(currentWindow->buffer + n,
				currentWindow->width + 3);
	n += currentWindow->width + 3;
    }

    free(currentWindow->buffer);

    if (currentWindow == windowStack) 
	currentWindow = NULL;
    else
	currentWindow--;

    SLsmg_set_char_set(0);

    newtRefresh();
}

void newtGetrc(int * row, int * col) {
   *row = cursorRow;
   *col = cursorCol;
}

void newtGotorc(int newRow, int newCol) {
    if (currentWindow) {
	newRow += currentWindow->top;
	newCol += currentWindow->left;
    }

    cursorRow = newRow;
    cursorCol = newCol;
    SLsmg_gotorc(cursorRow, cursorCol);
}

void newtDrawBox(int left, int top, int width, int height, int shadow) {
    if (currentWindow) {
	top += currentWindow->top;
	left += currentWindow->left;
    }

    SLsmg_draw_box(top, left, height, width);

    if (shadow) {
	SLsmg_set_color(NEWT_COLORSET_SHADOW);
	SLsmg_fill_region(top + height, left + 1, 1, width - 1, ' ');
	SLsmg_fill_region(top + 1, left + width, height, 1, ' ');
    }
}

void newtClearBox(int left, int top, int width, int height) {
    if (currentWindow) {
	top += currentWindow->top;
	left += currentWindow->left;
    }

    SLsmg_fill_region(top, left, height, width, ' ');
}

#if 0    
/* This doesn't seem to work quite right. I don't know why not, but when
   I rsh from an rxvt into a box and run this code, the machine returns
   console key's (\033[B) rather then xterm ones (\033OB). */
static void initKeymap(void) {
    struct keymap * curr;

    for (curr = keymap; curr->code; curr++) {
	if (!curr->str)
	    curr->str = SLtt_tgetstr(curr->tc);
    }

    /* Newt's keymap handling is a bit broken. It assumes that any extended
       keystrokes begin with ESC. If you're using a homebrek terminal you
       will probably need to fix this, or just yell at me and I'll be so
       ashamed of myself for doing it this way I'll fix it */

    keyPrefix = 0x1b;		/* ESC */
}
#endif

void newtDelay(int usecs) {
    fd_set set;
    struct timeval tv;

    FD_ZERO(&set);

    tv.tv_sec = usecs / 1000000;
    tv.tv_usec = usecs % 1000000;

    select(0, &set, &set, &set, &tv);
}

struct eventResult newtDefaultEventHandler(struct newtComponent * c, 
					   struct event ev) {
    struct eventResult er;

    er.result = ER_IGNORED;
    return er;
}

void newtRedrawHelpLine(void) {
    char * buf;

    SLsmg_set_color(NEWT_COLORSET_HELPLINE);
   
    buf = alloca(SLtt_Screen_Cols + 1);
    memset(buf, ' ', SLtt_Screen_Cols);
    buf[SLtt_Screen_Cols] = '\0';

    if (currentHelpline)
	memcpy(buf, *currentHelpline, strlen(*currentHelpline));

    SLsmg_gotorc(SLtt_Screen_Rows - 1, 0);
    SLsmg_write_string(buf);
}

void newtPushHelpLine(char * text) {
    if (!text)
	text = defaultHelpLine;
    
    if (currentHelpline)
	(*(++currentHelpline)) = strdup(text);
    else {
	currentHelpline = helplineStack;
	*currentHelpline = strdup(text);
    }

    newtRedrawHelpLine();
}

void newtPopHelpLine(void) {
    if (!currentHelpline) return;

    free(*currentHelpline);
    if (currentHelpline == helplineStack)
	currentHelpline = NULL;
    else
	currentHelpline--;

    newtRedrawHelpLine();
}

void newtDrawRootText(int row, int col, char * text) {
    SLsmg_set_color(NEWT_COLORSET_ROOTTEXT);

    if (col < 0) {
	col = SLtt_Screen_Cols + col;
    }

    if (row < 0) {
	col = SLtt_Screen_Cols + col;
    }
   
    SLsmg_gotorc(row, col);
    SLsmg_write_string(text);
}

int newtSetFlags(int oldFlags, int newFlags, enum newtFlagsSense sense) {
    switch (sense) {
      case NEWT_FLAGS_SET:
	return oldFlags | newFlags;

      case NEWT_FLAGS_RESET:
	return oldFlags & (~newFlags); 

      default:
	return oldFlags;
    }
}
