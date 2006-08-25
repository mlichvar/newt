#include "config.h"

#include <slang.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include "newt.h"
#include "newt_pr.h"

#if defined(HAVE_FRIBIDI_FRIBIDI_H) && defined(HAVE_DLFCN_H)
#include <fribidi/fribidi.h>
#include <dlfcn.h>

/* No sense in enabling shaping if we don't have BIDI support. */
typedef struct
{
	int is_shaped;
	wchar_t isolated;
	wchar_t initial;
	wchar_t medial;
	wchar_t final;
}
arabic_char_node;

#define ARABIC_BASE	0x621
#define ARABIC_END	0x64A

static const arabic_char_node arabic_shaping_table[] = {
/* 0x621 */  { TRUE , 0xFE80, 0x0000, 0x0000, 0x0000},
/* 0x622 */  { TRUE , 0xFE81, 0x0000, 0x0000, 0xFE82},
/* 0x623 */  { TRUE , 0xFE83, 0x0000, 0x0000, 0xFE84},
/* 0x624 */  { TRUE , 0xFE85, 0x0000, 0x0000, 0xFE86},
/* 0x625 */  { TRUE , 0xFE87, 0x0000, 0x0000, 0xFE88},
/* 0x626 */  { TRUE , 0xFE89, 0xFE8B, 0xFE8C, 0xFE8A},
/* 0x627 */  { TRUE , 0xFE8D, 0x0000, 0x0000, 0xFE8E},
/* 0x628 */  { TRUE , 0xFE8F, 0xFE91, 0xFE92, 0xFE90},
/* 0x629 */  { TRUE , 0xFE93, 0x0000, 0x0000, 0xFE94},
/* 0x62A */  { TRUE , 0xFE95, 0xFE97, 0xFE98, 0xFE96},
/* 0x62B */  { TRUE , 0xFE99, 0xFE9B, 0xFE9C, 0xFE9A},
/* 0x62C */  { TRUE , 0xFE9D, 0xFE9F, 0xFEA0, 0xFE9E},
/* 0x62D */  { TRUE , 0xFEA1, 0xFEA3, 0xFEA4, 0xFEA2},
/* 0x62E */  { TRUE , 0xFEA5, 0xFEA7, 0xFEA8, 0xFEA6},
/* 0x62F */  { TRUE , 0xFEA9, 0x0000, 0x0000, 0xFEAA},
/* 0x630 */  { TRUE , 0xFEAB, 0x0000, 0x0000, 0xFEAC},
/* 0x631 */  { TRUE , 0xFEAD, 0x0000, 0x0000, 0xFEAE},
/* 0x632 */  { TRUE , 0xFEAF, 0x0000, 0x0000, 0xFEB0},
/* 0x633 */  { TRUE , 0xFEB1, 0xFEB3, 0xFEB4, 0xFEB2},
/* 0x634 */  { TRUE , 0xFEB5, 0xFEB7, 0xFEB8, 0xFEB6},
/* 0x635 */  { TRUE , 0xFEB9, 0xFEBB, 0xFEBC, 0xFEBA},
/* 0x636 */  { TRUE , 0xFEBD, 0xFEBF, 0xFEC0, 0xFEBE},
/* 0x637 */  { TRUE , 0xFEC1, 0xFEC3, 0xFEC4, 0xFEC2},
/* 0x638 */  { TRUE , 0xFEC5, 0xFEC7, 0xFEC8, 0xFEC6},
/* 0x639 */  { TRUE , 0xFEC9, 0xFECB, 0xFECC, 0xFECA},
/* 0x63A */  { TRUE , 0xFECD, 0xFECF, 0xFED0, 0xFECE},
/* 0x63B */  { FALSE, 0x0000, 0x0000, 0x0000, 0x0000},
/* 0x63C */  { FALSE, 0x0000, 0x0000, 0x0000, 0x0000},
/* 0x63D */  { FALSE, 0x0000, 0x0000, 0x0000, 0x0000},
/* 0x63E */  { FALSE, 0x0000, 0x0000, 0x0000, 0x0000},
/* 0x63F */  { FALSE, 0x0000, 0x0000, 0x0000, 0x0000},
/* 0x640 */  { TRUE , 0x0640, 0x0640, 0x0640, 0x0640},
/* 0x641 */  { TRUE , 0xFED1, 0xFED3, 0xFED4, 0xFED2},
/* 0x642 */  { TRUE , 0xFED5, 0xFED7, 0xFED8, 0xFED6},
/* 0x643 */  { TRUE , 0xFED9, 0xFEDB, 0xFEDC, 0xFEDA},
/* 0x644 */  { TRUE , 0xFEDD, 0xFEDF, 0xFEE0, 0xFEDE},
/* 0x645 */  { TRUE , 0xFEE1, 0xFEE3, 0xFEE4, 0xFEE2},
/* 0x646 */  { TRUE , 0xFEE5, 0xFEE7, 0xFEE8, 0xFEE6},
/* 0x647 */  { TRUE , 0xFEE9, 0xFEEB, 0xFEEC, 0xFEEA},
/* 0x648 */  { TRUE , 0xFEED, 0x0000, 0x0000, 0xFEEE},
/* 0x649 */  { TRUE , 0xFEEF, 0x0000, 0x0000, 0xFEF0},
/* 0x64A */  { TRUE , 0xFEF1, 0xFEF3, 0xFEF4, 0xFEF2}
};

typedef struct {
	wchar_t c;
	arabic_char_node ar_node;
}
extra_char_node;

#define EXTRA_BASE 0x67E
#define EXTRA_END 0x6CC
static const extra_char_node extra_shaping_table[] = {
    {0x067E, {TRUE, 0xFB56, 0xFB58, 0xFB59, 0xFB57}},
    {0x0686, {TRUE, 0xFB7A, 0xFB7C, 0xFB7D, 0xFB7B}},
    {0x0698, {TRUE, 0xFB8A, 0x0000, 0x0000, 0xFB8B}},
    {0x06A9, {TRUE, 0xFB8E, 0xFB90, 0xFB91, 0xFB8F}},
    {0x06AF, {TRUE, 0xFB92, 0xFB94, 0xFB95, 0xFB93}},
    {0x06CC, {TRUE, 0xFBFC, 0xFBFE, 0xFBFF, 0xFBFD}},
    {0x0000, {FALSE, 0x0000, 0x0000, 0x0000, 0x0000}},
};

static const arabic_char_node *get_char_node(wchar_t w)
{
	if (w >= ARABIC_BASE && w <= ARABIC_END)
		return &arabic_shaping_table[w - ARABIC_BASE];
	else if (w >= EXTRA_BASE && w <= EXTRA_END) {
		const extra_char_node *node = extra_shaping_table;
		
		while (node->c) {
			if (node->c == w)
				return &node->ar_node;
			node++;
		}
		return NULL;
	}
	return NULL;
}

static int do_shaping(wchar_t *buf, int len) {
    int i,j;

    wchar_t *newbuf;

    if (len < 1)
	return 0;

    newbuf = (wchar_t *)malloc(sizeof(wchar_t)*len);

    for (i = 0, j = 0; i < len; i++, j++) {
	int have_previous = FALSE, have_next = FALSE;
	const arabic_char_node *node, *node1;
	int prev, next;
	
	if (buf[i] == L'\0')
	    break;

	/* If it is non-joiner, ignore it */
	if (buf[i] == 0x200C) {
	    j--;
	    continue;
	}
	
	newbuf[j] = buf[i];

	/* If it's not in our range, skip it. */
	node = get_char_node(buf[i]);
	if (!node)
	{
	    continue;
	}

	/* The character wasn't included in the unicode shaping table. */
	if (!node->is_shaped)
	{
	    continue;
	}
	
	for (prev = i - 1; prev >= 0; prev--)
	    if (wcwidth(buf[prev]) || buf[prev] == 0x200C)
	       break;
	
	if (prev >= 0 && (node1 = get_char_node(buf[prev]))
		&& ( node1->initial || node1->medial))
	{
	    have_previous = TRUE;
	}
	
	for (next = i + 1; next < len; next++)
 	    if (wcwidth(buf[next]) || buf[next] == 0x200C)
	       break;
	
	if (next < len && (node1 = get_char_node(buf[next]))
	    && (node1->medial || node1->final))
	{
	    have_next = TRUE;
	}

	/*
	 * FIXME: do not make ligature if there are combining 
	 * characters between two parts.
	 */
	if (buf[i] == 0x644 && have_next && next == i + 1)
	{
	    switch (buf[next])
	    {
		case 0x622:
		    newbuf[j] = 0xFEF5 + (have_previous ? 1 : 0);
		    i++;
		    continue;
		case 0x623:
		    newbuf[j] = 0xFEF7 + (have_previous ? 1 : 0);
		    i++;
		    continue;
		case 0x625:
		    newbuf[j] = 0xFEF9 + (have_previous ? 1 : 0);
		    i++;
		    continue;
		case 0x627:
		    newbuf[j] = 0xFEFB + (have_previous ? 1 : 0);
		    i++;
		    continue;
		default:
		    break;
	    }
	}

	/** Medial **/
	if (have_previous && have_next && node->medial)
	{
	    newbuf[j] = node->medial;
	}

	/** Final **/
	else if (have_previous && node->final)
	{
	    newbuf[j] = node->final;
	}

	/** Initial **/
	else if (have_next && node->initial)
	{
	    newbuf[j] = node->initial;
	}

	/** Isolated **/
	else if (node->isolated)
	{
	    newbuf[j] = node->isolated;
	}
    }
    for (i = 0; i < len && i < j; i++) {
	buf[i] = newbuf[i];
    }
    while (i < len) {
	buf[i++] = L'\0';
    }

    free(newbuf);
    return 0;
}

/* Converts bidi wchar text {in} to visual wchar text which is displayable 
 * in text mode. Uses {base_dir} as default base direction.
 * Returns malloc'ed converted text or NULL in case of error or if {need_out} is
 * not set. Modifies {base_dir} to reflect actual direction.
 */
static wchar_t* wchar_to_textmod_visual(wchar_t *in,unsigned int len,FriBidiCharType *base_dir, int need_out)
{
    FriBidiChar *out = NULL;
    static void *handle = NULL;

    fribidi_boolean (*func_ptr) (FriBidiChar *, FriBidiStrIndex,
                                 FriBidiCharType *, FriBidiChar *,
                                 FriBidiStrIndex *, FriBidiStrIndex *,
                                 FriBidiLevel *);

    if (!handle)
	handle = dlopen("/usr/lib/libfribidi.so.0", RTLD_LAZY | RTLD_GLOBAL);
    if (!handle)
	handle = dlopen("/lib/libfribidi.so.0", RTLD_LAZY | RTLD_GLOBAL);
    if (!handle)
	return NULL;

    func_ptr = dlsym(handle, "fribidi_log2vis");
    if (!func_ptr) {
	dlclose(handle);
	handle = NULL;
	return NULL;
    }
    
    if (need_out) {
        out = (FriBidiChar *)malloc(sizeof(FriBidiChar)*(len+1));
        if(!out)
        {
	    dlclose(handle);
	    handle = NULL;
            return (wchar_t *) out;
        }

        do_shaping(in, len);
	len = wcsnlen(in, len);
    }
    (*func_ptr)(in, len, base_dir, out, NULL, NULL, NULL);

    return (wchar_t *) out;
}

/*
 * Converts text given in {str} from logical order to visual order.
 * Uses {dir} as base direction ('N', 'R', 'L').
 * Returns malloc'ed converted string. Modifies {dir} to reflect actual
 * direction.
 */
static char *_newt_log2vis(const char *str, char *dir)
{
	wchar_t *wcs;
	char *rstr = NULL;
	int len = strlen(str);
	int ret;
	FriBidiCharType basedir;

	switch (*dir)
	{
		case 'R': basedir = FRIBIDI_TYPE_R;
			  break;
		case 'L': basedir = FRIBIDI_TYPE_L;
			  break;
		default: basedir = FRIBIDI_TYPE_ON;
			 break;
	}
	
	if (len) {
		wchar_t *owcs;
		int newlen;
		
		wcs = malloc(sizeof(*wcs) * (len + 1));
		if (!wcs)
			return NULL;
		ret = mbstowcs(wcs, str, len + 1);
		if (ret < 0) {
			free(wcs);
			return NULL;
		}
		owcs = wchar_to_textmod_visual(wcs, ret, &basedir, 1);
		if (FRIBIDI_DIR_TO_LEVEL(basedir))
			*dir = 'R';
		else
			*dir = 'L';
		
		free(wcs);
		if (!owcs)
			return NULL;
		
		newlen = wcstombs(NULL, owcs, 0);
		if (newlen < 0) {
			free(owcs);
			return NULL;
		}
		rstr = malloc(newlen + 1);
		if (!rstr) {
			free(owcs);
			return NULL;
		}
		ret = wcstombs(rstr, owcs, newlen + 1);
		free(owcs);
		if (ret < 0) {
			free(rstr);
			return NULL;
		}
	}
	return rstr;
}

/* Returns base direction of text given in {str}.
 */
char get_text_direction(const char *str)
{
	int len = strlen(str);
	char dir = 'N';
	wchar_t *wcs;
	int ret;
	
	FriBidiCharType basedir = FRIBIDI_TYPE_ON;
	
	if (len) {
		wcs = malloc(sizeof(*wcs) * (len + 1));
		if (!wcs)
			return dir;
		ret = mbstowcs(wcs, str, len + 1);
		if (ret < 0) {
			free(wcs);
			return dir;
		}
		wchar_to_textmod_visual(wcs, ret, &basedir, 1);
		free(wcs);
		if (FRIBIDI_DIR_TO_LEVEL(basedir))
			dir = 'R';
		else
			dir = 'L';
	}
	return dir;
}

/* If width of string {str} is less then {width} adds
 * final spaces to make it {width} position wide.
 * Returns malloc'ed padded string or NULL in case of errors 
 * or if string does not need padding.
 */
static char *pad_line(const char *str, int width)
{
	int len = strlen(str);
	int w = _newt_wstrlen(str, len);

	if (w < width) {
		char *newstr = malloc(len + 1 + (width - w));
		if (!newstr)
			return NULL;
		memcpy(newstr, str, len);
		memset(newstr + len, ' ', width - w);
		newstr[len+width-w] = '\0';
		return newstr;
	}
	return NULL;
}

/*
 * Writes string {str}. Uses {dir} as default direction.
 * Returns direction of the string in {dir}.
 */
void write_string_int(const char *str, char *dir)
{
	char dummy;
	char *tmpstr;

	if (!dir) {
		dummy = 'N';
		dir = &dummy;
	}
	
	tmpstr = _newt_log2vis(str, dir);
	if (tmpstr)
		str = tmpstr;
	SLsmg_write_string(str);
	if (tmpstr)
		free(tmpstr);
}

/* Writes at most {n} positions of the string {str}.
 * Adds final (logical) spaces if string width is less than {n}.
 * Uses {dir} as default direction.
 * Returns direction of the string in {dir}
 */
void write_nstring_int(const char *str, int n, char *dir)
{
	char dummy;
	char *tmpstr, *tmpstr1;

	if (!dir) {
		dummy = 'N';
		dir = &dummy;
	}

	tmpstr1 = pad_line(str, n);
	if (tmpstr1)
		str = tmpstr1;
	tmpstr = _newt_log2vis(str, dir);
	if (tmpstr) {
		free(tmpstr1);
		str = tmpstr;
	}
	SLsmg_write_nstring(str, n);
	if (tmpstr)
		free(tmpstr);
	else
		free(tmpstr1);
}
#else
void write_string_int(const char *str, char *dir)
{
	SLsmg_write_string(str);
}

void write_nstring_int(const char *str, int w, char *dir)
{
	SLsmg_write_nstring(str, w);
}

char get_text_direction(const char *str)
{
	return 'L';
}
#endif

struct Window {
    int height, width, top, left;
    SLsmg_Char_Type * buffer;
    char * title;
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
static int needResize = 0;
static int cursorOn = 1;
static int trashScreen = 0;

static const char * defaultHelpLine =
"  <Tab>/<Alt-Tab> between elements   |  <Space> selects   |  <F12> next screen"
;

const struct newtColors newtDefaultColorPalette = {
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
	"black", "lightgray",			/* compact button fg, bg */
	"yellow", "red",			/* active & sel listbox */
	"black", "brown"			/* selected listbox */
};

static const struct keymap keymap[] = {
	{ "\033OA", 		NEWT_KEY_UP, 		"ku" },
	{ "\020", 		NEWT_KEY_UP, 		NULL }, /* emacs ^P */
	{ "\033OB", 		NEWT_KEY_DOWN, 		"kd" },
	{ "\016", 		NEWT_KEY_DOWN, 		NULL }, /* emacs ^N */
	{ "\033OC", 		NEWT_KEY_RIGHT, 	"kr" },
	{ "\006", 		NEWT_KEY_RIGHT, 	NULL }, /* emacs ^F */
	{ "\033OD", 		NEWT_KEY_LEFT, 		"kl" },
	{ "\002", 		NEWT_KEY_LEFT, 		NULL }, /* emacs ^B */
	{ "\033OH",		NEWT_KEY_HOME, 		"kh" },
	{ "\033[1~",		NEWT_KEY_HOME, 		NULL },
	{ "\001",		NEWT_KEY_HOME, 		NULL }, /* emacs ^A */
	{ "\033Ow",		NEWT_KEY_END, 		"kH" },
        { "\033[4~",		NEWT_KEY_END, 		"@7" },
        { "\005",		NEWT_KEY_END, 		NULL }, /* emacs ^E */

	{ "\033[3~",		NEWT_KEY_DELETE,	"kD" },
	{ "\004",		NEWT_KEY_DELETE,	NULL }, /* emacs ^D */
	{ "\033[2~", 		NEWT_KEY_INSERT,        "kI" },

	{ "\033\t",		NEWT_KEY_UNTAB,		"kB" },

	{ "\033[5~",		NEWT_KEY_PGUP,		"kP" },
	{ "\033[6~",		NEWT_KEY_PGDN,		"kN" },
	{ "\033V",		NEWT_KEY_PGUP, 		NULL },
	{ "\033v",		NEWT_KEY_PGUP, 		NULL },
        { "\026",		NEWT_KEY_PGDN,		NULL },

	{ "\033[[A",		NEWT_KEY_F1,		NULL },
	{ "\033[[B",		NEWT_KEY_F2,		NULL },
	{ "\033[[C",		NEWT_KEY_F3,		NULL },
	{ "\033[[D",		NEWT_KEY_F4,		NULL },
	{ "\033[[E",		NEWT_KEY_F5,		NULL },

	{ "\033OP",		NEWT_KEY_F1,		NULL },
	{ "\033OQ",		NEWT_KEY_F2,		NULL },
	{ "\033OR",		NEWT_KEY_F3,		NULL },
	{ "\033OS",		NEWT_KEY_F4,		NULL },

	{ "\033[11~",		NEWT_KEY_F1,		"k1" },
	{ "\033[12~",		NEWT_KEY_F2,		"k2" },
	{ "\033[13~",		NEWT_KEY_F3,		"k3" },
	{ "\033[14~",		NEWT_KEY_F4,		"k4" },
	{ "\033[15~",		NEWT_KEY_F5,		"k5" },
	{ "\033[17~",		NEWT_KEY_F6,		"k6" },
	{ "\033[18~",		NEWT_KEY_F7,		"k7" },
	{ "\033[19~",		NEWT_KEY_F8,		"k8" },
	{ "\033[20~",		NEWT_KEY_F9,            "k9" },
	{ "\033[21~",		NEWT_KEY_F10,		"k;" },
	{ "\033[23~",		NEWT_KEY_F11,		"F1" },
	{ "\033[24~",		NEWT_KEY_F12,		"F2" },
	{ "\033",		NEWT_KEY_ESCAPE,	"@2" },
        { "\033",		NEWT_KEY_ESCAPE,	"@9" },

        { "\177",		NEWT_KEY_BKSPC,		NULL },
        { "\010",		NEWT_KEY_BKSPC,		NULL },
        
	{ 0 },	/* LEAVE this one */
};
static void initKeymap();

static const char ident[] = // ident friendly
    "$Version: Newt windowing library v" VERSION " $"
    "$Copyright: (C) 1996-2003 Red Hat, Inc. Written by Erik Troan $"
    "$License: Lesser GNU Public License. $";

static newtSuspendCallback suspendCallback = NULL;
static void * suspendCallbackData = NULL;

void newtSetSuspendCallback(newtSuspendCallback cb, void * data) {
    suspendCallback = cb;
    suspendCallbackData = data;
}

static void handleSigwinch(int signum) {
    needResize = 1;
}

static int getkeyInterruptHook(void) {
    return -1;
}

int _newt_wstrlen(const char *str, int len) {
	mbstate_t ps;
	wchar_t tmp;
	int nchars = 0;
	
	if (!str) return 0;
	if (!len) return 0;
	if (len < 0) len = strlen(str);
	memset(&ps,0,sizeof(mbstate_t));
	while (len > 0) {
		int x,y;
		
		x = mbrtowc(&tmp,str,len,&ps);
		if (x >0) {
		    	str += x;
			len -= x;
			y = wcwidth(tmp);
			if (y>0)
			  nchars+=y;
		} else break;
	}
	return nchars;
}

/** Trim a string to fit 
 * @param title - string. NULL will be inserted if necessary
 * @param chrs  - available space. (character cells)
 */
void trim_string(char *title, int chrs)
{
	char *p = title;
	int ln = chrs;
	int x = 0,y = 0;
	wchar_t tmp;
	mbstate_t ps;

	memset(&ps, 0, sizeof(ps));

	while (*p) {
		x = mbrtowc(&tmp, p, ln, &ps);
		if (x < 0) { // error
			*p = '\0';
			return;
		}
		y = wcwidth(tmp);
		if (y > ln) {
			*p = '\0';
			return;
		} else {
			p += x;
			ln -= y;
		}
	}	
}

static int getkey() {
    int c;

    while ((c = SLang_getkey()) == '\xC') { /* if Ctrl-L redraw whole screen */
        SLsmg_touch_lines (0, SLtt_Screen_Rows - 1);
        SLsmg_refresh();
    }
    return c;

}

void newtFlushInput(void) {
    while (SLang_input_pending(0)) {
	getkey();
    }
}

/**
 * @brief Refresh the screen
 */
void newtRefresh(void) {
    SLsmg_refresh();
}

void newtSuspend(void) {
    SLtt_set_cursor_visibility (1);
    SLsmg_suspend_smg();
    SLang_reset_tty();
    SLtt_set_cursor_visibility (cursorOn);
}

/**
 *  @brief Return after suspension.
 *  @return 0 on success.
 */
int newtResume(void) {
    SLsmg_resume_smg ();
    SLsmg_refresh();
    return SLang_init_tty(0, 0, 0);
}

void newtCls(void) {
    SLsmg_set_color(NEWT_COLORSET_ROOT);
    SLsmg_gotorc(0, 0);
    SLsmg_erase_eos();

    newtRefresh();
}

/**
 * @brief Resize the screen
 * @param redraw - boolean - should we redraw the screen?
 */
void newtResizeScreen(int redraw) {
    SLtt_get_screen_size();
    SLsmg_reinit_smg();
    if (redraw) {
        SLsmg_touch_lines (0, SLtt_Screen_Rows - 1);
        newtRefresh();
    }
}

/**
 * @brief Initialize the newt library
 * @return int - 0 for success, else < 0
 */
int newtInit(void) {
    char * MonoValue, * MonoEnv = "NEWT_MONO";
    const char *lang;
    int ret;

    if ((lang = getenv("LC_ALL")) == NULL)
        if ((lang = getenv("LC_CTYPE")) == NULL)
            if ((lang = getenv("LANG")) == NULL)
                lang = "";
    if (strstr (lang, ".euc") != NULL)
	trashScreen = 1;

    (void) strlen(ident);

    SLtt_get_terminfo();
    SLtt_get_screen_size();
    // there is no such function in slang?!
    // SLutf8_enable(-1); /* init. utf8 according to locale */

    MonoValue = getenv(MonoEnv);
    if ( MonoValue == NULL ) {
	SLtt_Use_Ansi_Colors = 1;
    } else {
	SLtt_Use_Ansi_Colors = 0;
    }

    if ((ret = SLsmg_init_smg()) < 0)
	return ret;
    if ((ret = SLang_init_tty(0, 0, 0)) < 0)
	return ret;

    newtSetColors(newtDefaultColorPalette);
    newtCursorOff();
    initKeymap();

    /*memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handleSigwinch;
    sigaction(SIGWINCH, &sa, NULL);*/

    SLsignal_intr(SIGWINCH, handleSigwinch);
    SLang_getkey_intr_hook = getkeyInterruptHook;

    return 0;
}

/**
 * @brief Closedown the newt library, tidying screen.
 * @returns int , 0. (no errors reported)
 */
int newtFinished(void) {
    SLsmg_gotorc(SLtt_Screen_Rows - 1, 0);
    newtCursorOn();
    SLsmg_refresh();
    SLsmg_reset_smg();
    SLang_reset_tty();

    return 0;
}

/**
 * @brief Set the colors used.
 * @param colors - newtColor struct used.
 */
void newtSetColors(struct newtColors colors) {
    if (!SLtt_Use_Ansi_Colors) {
        int i;

        for (i = 2; i < 25; i++)
            SLtt_set_mono(i, NULL, 0);

        SLtt_set_mono(NEWT_COLORSET_SELLISTBOX, NULL, SLTT_BOLD_MASK);

        SLtt_set_mono(NEWT_COLORSET_ACTBUTTON, NULL, SLTT_REV_MASK);
        SLtt_set_mono(NEWT_COLORSET_ACTCHECKBOX, NULL, SLTT_REV_MASK);
        SLtt_set_mono(NEWT_COLORSET_ACTLISTBOX, NULL, SLTT_REV_MASK);
        SLtt_set_mono(NEWT_COLORSET_ACTTEXTBOX, NULL, SLTT_REV_MASK);

        SLtt_set_mono(NEWT_COLORSET_ACTSELLISTBOX, NULL, SLTT_REV_MASK | SLTT_BOLD_MASK);
        
        SLtt_set_mono(NEWT_COLORSET_DISENTRY, NULL, 0); // FIXME
        SLtt_set_mono(NEWT_COLORSET_FULLSCALE, NULL, SLTT_ULINE_MASK | SLTT_REV_MASK);
        SLtt_set_mono(NEWT_COLORSET_EMPTYSCALE, NULL, SLTT_ULINE_MASK);
        return;
    }
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

    SLtt_set_color(NEWT_COLORSET_EMPTYSCALE, "", "white",
			colors.emptyScale);
    SLtt_set_color(NEWT_COLORSET_FULLSCALE, "", "white",
			colors.fullScale);
    SLtt_set_color(NEWT_COLORSET_DISENTRY, "", colors.disabledEntryFg,
			colors.disabledEntryBg);

    SLtt_set_color(NEWT_COLORSET_COMPACTBUTTON, "", colors.compactButtonFg,
			colors.compactButtonBg);

    SLtt_set_color(NEWT_COLORSET_ACTSELLISTBOX, "", colors.actSelListboxFg,
		   colors.actSelListboxBg);
    SLtt_set_color(NEWT_COLORSET_SELLISTBOX, "", colors.selListboxFg,
		   colors.selListboxBg);
}

/* Keymap handling - rewritten by Henning Makholm <henning@makholm.net>,
 * November 2003.
 */

struct kmap_trie_entry {
    char c ;   /* character got from terminal */
    int code;  /* newt key, or 0 if c does not make a complete sequence */
    struct kmap_trie_entry *contseq; /* sub-trie for character following c */
    struct kmap_trie_entry *next;    /* try this if char received != c */
};
/* Here are some static entries that will help in handling esc O foo and
   esc [ foo as variants of each other: */
static struct kmap_trie_entry
    kmap_trie_escO     = { 'O', 0, 0, 0 },
    kmap_trie_escBrack = { '[', 0, 0, &kmap_trie_escO },
    kmap_trie_root     = { '\033', 0, &kmap_trie_escBrack, 0 };
static int keyreader_buf_len = 10 ;
static unsigned char default_keyreader_buf[10];
static unsigned char *keyreader_buf = default_keyreader_buf;

#if 0 /* for testing of the keymap manipulation code */
static void dumpkeys_recursive(struct kmap_trie_entry *curr, int i, FILE *f) {
    int j, ps ;
    char seen[256]={0};
    if( curr && i >= keyreader_buf_len ) {
        fprintf(f,"ARGH! Too long sequence!\n") ;
        return ;
    }
    for(;curr;curr=curr->next) {
        keyreader_buf[i] = curr->c ;
        ps = seen[(unsigned char)curr->c]++ ;
        if( ps || curr->code || (!curr->code && !curr->contseq) ) {
            for(j=0;j<=i;j++) {
                if( keyreader_buf[j] > 32 && keyreader_buf[j]<127 &&
                    keyreader_buf[j] != '^' && keyreader_buf[j] != '\\' )
                    fprintf(f,"%c",keyreader_buf[j]);
                else if( keyreader_buf[j] > 0 && keyreader_buf[j]<=32 )
                    fprintf(f,"^%c",keyreader_buf[j] + 0x40);
                else
                    fprintf(f,"\\%03o",
                            (unsigned)(unsigned char)keyreader_buf[j]);
            }
            if( curr->code )
                fprintf(f,": 0x%X\n",curr->code);
            else
                fprintf(f,": (just keymap)\n");
        }
        dumpkeys_recursive(curr->contseq,i+1,f);
    }
}
static void dump_keymap(void) {
    FILE *f = fopen("newt.keydump","wt");
    if (f) {
        dumpkeys_recursive(&kmap_trie_root,0,f);
        fclose(f);
    }
}
#endif

/* newtBindKey may overwrite a binding that is there already */
static void newtBindKey(char *keyseq, int meaning) {
    struct kmap_trie_entry *root = &kmap_trie_root ;
    struct kmap_trie_entry **curptr = &root ;

    /* Try to make sure the common matching buffer is long enough. */
    if( strlen(keyseq) > keyreader_buf_len ) {
        int i = strlen(keyseq)+10;
        unsigned char *newbuf = malloc(i);
        if (newbuf) {
            if (keyreader_buf != default_keyreader_buf)
                free(keyreader_buf);
            keyreader_buf = newbuf;
            keyreader_buf_len = i;
        }
    }
    
    if (*keyseq == 0) return; /* binding the empty sequence is meaningless */
    
    while(1) {
        while ((*curptr) && (*curptr)->c != *keyseq)
            curptr = &(*curptr)->next;
        if ((*curptr)==0) {
            struct kmap_trie_entry* fresh
                =  calloc(strlen(keyseq),sizeof(struct kmap_trie_entry));
            if (fresh == 0) return; /* despair! */
            *curptr = fresh;
            while (keyseq[1]) {
                fresh->contseq = fresh+1;
                (fresh++)->c = *(keyseq++);
            }
            fresh->c = *keyseq;
            fresh->code = meaning;
            return;
        }
        if (keyseq[1]==0) {
            (*curptr)->code = meaning;
            return;
        } else {
            curptr = &(*curptr)->contseq;
            keyseq++;
        }
    }      
}

/* This function recursively inserts all entries in the "to" trie into
   corresponding positions in the "from" trie, except positions that
   are already defined in the "from" trie. */
static void kmap_trie_fallback(struct kmap_trie_entry *to,
                               struct kmap_trie_entry **from) {
    if (*from == NULL)
        *from = to ;
    if (*from == to)
        return ;
    for (;to!=NULL;to=to->next) {
        struct kmap_trie_entry **fromcopy = from ;
        while ((*fromcopy) && (*fromcopy)->c != to->c)
            fromcopy = &(*fromcopy)->next ;
        if (*fromcopy) {
            if ((*fromcopy)->code == 0)
                (*fromcopy)->code = to->code;
            kmap_trie_fallback(to->contseq, &(*fromcopy)->contseq);
        } else {
            *fromcopy = malloc(sizeof(struct kmap_trie_entry));
            if (*fromcopy) {
                **fromcopy = *to ;
                (*fromcopy)->next = 0 ;
            }
        }
    }
}

int newtGetKey(void) {
    int key;
    unsigned char *chptr = keyreader_buf, *lastmatch;
    int lastcode;
    struct kmap_trie_entry *curr = &kmap_trie_root;

    do {
	key = getkey();
	if (key == SLANG_GETKEY_ERROR) {
	    /* Either garbage was read, or stdin disappeared
	     * (the parent terminal was proably closed)
	     * if the latter, die.
	     */
	    if (feof(stdin))
		    exit(1);
	    if (needResize) {
                needResize = 0;
		return NEWT_KEY_RESIZE;
            }

	    /* ignore other signals */
	    continue;
	}

	if (key == NEWT_KEY_SUSPEND && suspendCallback)
	    suspendCallback(suspendCallbackData);
    } while (key == NEWT_KEY_SUSPEND);

    /* Read more characters, matching against the trie as we go */
    lastcode = *chptr = key;
    lastmatch = chptr ;
    while(1) {
         while (curr->c != key) {
             curr = curr->next ;
             if (curr==NULL) goto break2levels;
         }
         if (curr->code) {
             lastcode = curr->code;
             lastmatch = chptr;
         }
         curr = curr->contseq;
         if (curr==NULL) break;

         if (SLang_input_pending(5) <= 0)
             break;

         if (chptr==keyreader_buf+keyreader_buf_len-1) break;
         *++chptr = key = getkey();
    }
   break2levels:

      /* The last time the trie matched was at position lastmatch. Back
       * up if we have read too many characters. */
      while (chptr > lastmatch)
          SLang_ungetkey(*chptr--);
    
      return lastcode;
}

/**
 * @brief Wait for a keystroke
 */
void newtWaitForKey(void) {
    newtRefresh();

    getkey();
    newtClearKeyBuffer();
}

/**
 * @brief Clear the keybuffer
 */
void newtClearKeyBuffer(void) {
    while (SLang_input_pending(1)) {
	getkey();
    }
}

/**
 * Open a new window.
 * @param left. unsigned int Size; _not_ including border
 * @param top: unsigned int size, _not_ including border
 * @param width unsigned int
 * @param height unsigned int
 * @param title - title string
 * @return zero on success (currently no errors reported)
 */
int newtOpenWindow(unsigned int left, unsigned int top, 
                   unsigned int width, unsigned int height,
			  const char * title) {
    int j, row, col;
    int n;
    int i;

    newtFlushInput();

    if (!currentWindow) {
	currentWindow = windowStack;
    } else {
	currentWindow++;
    }

    currentWindow->left = left;
    currentWindow->top = top;
    currentWindow->width = width;
    currentWindow->height = height;
    currentWindow->title = title ? strdup(title) : NULL;

    currentWindow->buffer = malloc(sizeof(SLsmg_Char_Type) * (width + 3) * (height + 3));

    row = top - 1;
    col = left - 1;
    /* clip to the current screen bounds - msw */
    if (row < 0)
	row = 0;
    if (col < 0)
	col = 0;
    if (left + width > SLtt_Screen_Cols)
	width = SLtt_Screen_Cols - left;
    if (top + height > SLtt_Screen_Rows)
	height = SLtt_Screen_Rows - top;
    n = 0;
    for (j = 0; j < height + 3; j++, row++) {
	SLsmg_gotorc(row, col);
	SLsmg_read_raw(currentWindow->buffer + n,
		       currentWindow->width + 3);
	n += currentWindow->width + 3;
    }

    newtTrashScreen();

    SLsmg_set_color(NEWT_COLORSET_BORDER);
    SLsmg_set_char_set(1);
    SLsmg_draw_box(top - 1, left - 1, height + 2, width + 2);
    SLsmg_set_char_set(0);

    if (currentWindow->title) {
	trim_string (currentWindow->title, width-4);
	i = wstrlen(currentWindow->title,-1) + 4;
	i = ((width - i) / 2) + left;
	SLsmg_gotorc(top - 1, i);
	SLsmg_set_char_set(1);
	SLsmg_write_char(SLSMG_RTEE_CHAR);
	SLsmg_set_char_set(0);
	SLsmg_write_char(' ');
	SLsmg_set_color(NEWT_COLORSET_TITLE);
	write_string_int((char *)currentWindow->title, NULL);
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

/**
 * @brief Draw a centered window.
 * @param width - width in char cells
 * @param height - no. of char cells.
 * @param title - fixed title
 * @returns 0. No errors reported
 */
int newtCenteredWindow(unsigned int width,unsigned int height, 
                       const char * title) {
    unsigned int top, left;

    top = (SLtt_Screen_Rows - height) / 2;

    /* I don't know why, but this seems to look better */
    if ((SLtt_Screen_Rows % 2) && (top % 2)) top--;

    left = (SLtt_Screen_Cols - width) / 2;

    newtOpenWindow(left, top, width, height, title);

    return 0;
}

/**
 * @brief Remove the top window
 */
void newtPopWindow(void) {
    int j, row, col;
    int n = 0;

    row = col = 0;

    row = currentWindow->top - 1;
    col = currentWindow->left - 1;
    if (row < 0)
	row = 0;
    if (col < 0)
	col = 0;
    for (j = 0; j < currentWindow->height + 3; j++, row++) {
	SLsmg_gotorc(row, col);
	SLsmg_write_raw(currentWindow->buffer + n,
			currentWindow->width + 3);
	n += currentWindow->width + 3;
    }

    free(currentWindow->buffer);
    free(currentWindow->title);

    if (currentWindow == windowStack)
	currentWindow = NULL;
    else
	currentWindow--;

    SLsmg_set_char_set(0);

    newtTrashScreen();

    newtRefresh();
}

void newtGetWindowPos(int * x, int * y) {
    if (currentWindow) {
	*x = currentWindow->left;
	*y = currentWindow->top;
    } else
	*x = *y = 0;
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

static void initKeymap(void) {
    const struct keymap * curr;

    /* First bind built-in default bindings. They may be shadowed by
       the termcap entries that get bound later. */
    for (curr = keymap; curr->code; curr++) {
        if (curr->str)
            newtBindKey(curr->str,curr->code);
    }

    /* Then bind strings from termcap entries */
    for (curr = keymap; curr->code; curr++) {
	if (curr->tc) {
            char *pc = SLtt_tgetstr(curr->tc);
            if (pc) {
                newtBindKey(pc,curr->code);
            }
        }
    }

    /* Finally, invent lowest-priority keybindings that correspond to
       searching for esc-O-foo if esc-[-foo was not found and vice
       versa.  That is needed because of strong confusion among
       different emulators of VTxxx terminals; some terminfo/termcap
       descriptions are apparently written by people who were not
       aware of the differences between "applicataion" and "terminal"
       keypad modes. Or perhaps they were, but tried to make their
       description work with a program that puts the keyboard in the
       wrong emulation mode. In short, one needs this: */
    kmap_trie_fallback(kmap_trie_escO.contseq, &kmap_trie_escBrack.contseq);
    kmap_trie_fallback(kmap_trie_escBrack.contseq, &kmap_trie_escO.contseq);
}

/**
 * @brief Delay for a specified number of usecs
 * @param int - number of usecs to wait for.
 */
void newtDelay(unsigned int usecs) {
    fd_set set;
    struct timeval tv;

    FD_ZERO(&set);

    tv.tv_sec = usecs / 1000000;
    tv.tv_usec = usecs % 1000000;

    select(0, &set, &set, &set, &tv);
}

struct eventResult newtDefaultEventHandler(newtComponent c,
					   struct event ev) {
    struct eventResult er;

    er.result = ER_IGNORED;
    return er;
}

void newtRedrawHelpLine(void) {
    char * buf;

    SLsmg_set_color(NEWT_COLORSET_HELPLINE);

    if (currentHelpline) {
	/* buffer size needs to be wide enough to hold all the multibyte
	   currentHelpline + all the single byte ' ' to fill the line */
	int wlen = wstrlen(*currentHelpline, -1);
	int len;

	if (wlen > SLtt_Screen_Cols)
	    wlen = SLtt_Screen_Cols;
	len = strlen(*currentHelpline) + (SLtt_Screen_Cols - wlen);
	buf = alloca(len + 1);
	memset(buf, ' ', len);
	memcpy(buf, *currentHelpline, strlen(*currentHelpline));
	buf[len] = '\0';
    } else {
	buf = alloca(SLtt_Screen_Cols + 1);
	memset(buf, ' ', SLtt_Screen_Cols);
	buf[SLtt_Screen_Cols] = '\0';
    }
    SLsmg_gotorc(SLtt_Screen_Rows - 1, 0);
    write_string_int(buf, NULL);
}

void newtPushHelpLine(const char * text) {
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

void newtDrawRootText(int col, int row, const char * text) {
    SLsmg_set_color(NEWT_COLORSET_ROOTTEXT);

    if (col < 0) {
	col = SLtt_Screen_Cols + col;
    }

    if (row < 0) {
	row = SLtt_Screen_Rows + row;
    }

    SLsmg_gotorc(row, col);
    write_string_int((char *)text, NULL);
}

int newtSetFlags(int oldFlags, int newFlags, enum newtFlagsSense sense) {
    switch (sense) {
      case NEWT_FLAGS_SET:
	return oldFlags | newFlags;

      case NEWT_FLAGS_RESET:
	return oldFlags & (~newFlags);

      case NEWT_FLAGS_TOGGLE:
	return oldFlags ^ newFlags;

      default:
	return oldFlags;
    }
}

void newtBell(void)
{
    SLtt_beep();
}

void newtGetScreenSize(int * cols, int * rows) {
    if (rows) *rows = SLtt_Screen_Rows;
    if (cols) *cols = SLtt_Screen_Cols;
}

void newtDefaultPlaceHandler(newtComponent c, int newLeft, int newTop) {
    c->left = newLeft;
    c->top = newTop;
}

void newtDefaultMappedHandler(newtComponent c, int isMapped) {
    c->isMapped = isMapped;
}

void newtCursorOff(void) {
    cursorOn = 0;
    SLtt_set_cursor_visibility (cursorOn);
}

void newtCursorOn(void) {
    cursorOn = 1;
    SLtt_set_cursor_visibility (cursorOn);
}

void newtTrashScreen(void) {
    if (trashScreen)
	SLsmg_touch_lines (0, SLtt_Screen_Rows - 1);
}
     
