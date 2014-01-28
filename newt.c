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
static int cursorOn = 1;
static int trashScreen = 0;
extern int needResize;

static const char * const defaultHelpLine =
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
	"lightgray", "blue",			/* checkbox fg, bg */
	"lightgray", "red",			/* active checkbox fg, bg */
	"lightgray", "blue",			/* entry box fg, bg */
	"blue", "lightgray",			/* label fg, bg */
	"black", "lightgray",			/* listbox fg, bg */
	"lightgray", "blue",			/* active listbox fg, bg */
	"black", "lightgray",			/* textbox fg, bg */
	"lightgray", "red",			/* active textbox fg, bg */
	"white", "blue",			/* help line */
	"lightgray", "blue",			/* root text */
	"blue",					/* scale full */
	"red",					/* scale empty */
	"blue", "lightgray",			/* disabled entry fg, bg */
	"black", "lightgray",			/* compact button fg, bg */
	"lightgray", "red",			/* active & sel listbox */
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
	{ "\033[Z",		NEWT_KEY_UNTAB,		NULL },

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
static void freeKeymap();

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
	int ln;
	int x = 0,y = 0;
	wchar_t tmp;
	mbstate_t ps;

	memset(&ps, 0, sizeof(ps));
	ln = strlen(title);

	while (*p) {
		x = mbrtowc(&tmp, p, ln, &ps);
		if (x < 0) { // error
			*p = '\0';
			return;
		}
		y = wcwidth(tmp);
		if (y > chrs) {
			*p = '\0';
			return;
		} else {
			p += x;
			ln -= x;
			chrs -= y;
		}
	}	
}

static int getkey() {
    int c;

    while ((c = SLang_getkey()) == '\xC') { /* if Ctrl-L redraw whole screen */
        SLsmg_touch_lines(0, SLtt_Screen_Rows);
        SLsmg_refresh();
    }
    return c;

}

static void updateColorset(char *fg, char *bg, char **fg_p, char **bg_p)
{
    if (*fg && fg_p)
	*fg_p = fg;
    if (*bg && bg_p)
	*bg_p = bg;
}

/* parse color specifications (e.g. root=,black:border=red,blue)
 * and update the palette
 */
static void parseColors(char *s, struct newtColors *palette)
{
    char *name, *str, *fg, *bg;

    for (str = s; (s = strtok(str, ";:\n\r\t ")); str = NULL) {
	name = s;
	if (!(s = strchr(s, '=')) || !*s)
	    continue;
	*s = '\0';
	fg = ++s;
	if (!(s = strchr(s, ',')) || !*s)
	    continue;
	*s = '\0';
	bg = ++s;

	if (!strcmp(name, "root"))
	    updateColorset(fg, bg, &palette->rootFg, &palette->rootBg);
	else if (!strcmp(name, "border"))
	    updateColorset(fg, bg, &palette->borderFg, &palette->borderBg);
	else if (!strcmp(name, "window"))
	    updateColorset(fg, bg, &palette->windowFg, &palette->windowBg);
	else if (!strcmp(name, "shadow"))
	    updateColorset(fg, bg, &palette->shadowFg, &palette->shadowBg);
	else if (!strcmp(name, "title"))
	    updateColorset(fg, bg, &palette->titleFg, &palette->titleBg);
	else if (!strcmp(name, "button"))
	    updateColorset(fg, bg, &palette->buttonFg, &palette->buttonBg);
	else if (!strcmp(name, "actbutton"))
	    updateColorset(fg, bg, &palette->actButtonFg, &palette->actButtonBg);
	else if (!strcmp(name, "checkbox"))
	    updateColorset(fg, bg, &palette->checkboxFg, &palette->checkboxBg);
	else if (!strcmp(name, "actcheckbox"))
	    updateColorset(fg, bg, &palette->actCheckboxFg, &palette->actCheckboxBg);
	else if (!strcmp(name, "entry"))
	    updateColorset(fg, bg, &palette->entryFg, &palette->entryBg);
	else if (!strcmp(name, "label"))
	    updateColorset(fg, bg, &palette->labelFg, &palette->labelBg);
	else if (!strcmp(name, "listbox"))
	    updateColorset(fg, bg, &palette->listboxFg, &palette->listboxBg);
	else if (!strcmp(name, "actlistbox"))
	    updateColorset(fg, bg, &palette->actListboxFg, &palette->actListboxBg);
	else if (!strcmp(name, "textbox"))
	    updateColorset(fg, bg, &palette->textboxFg, &palette->textboxBg);
	else if (!strcmp(name, "acttextbox"))
	    updateColorset(fg, bg, &palette->actTextboxFg, &palette->actTextboxBg);
	else if (!strcmp(name, "helpline"))
	    updateColorset(fg, bg, &palette->helpLineFg, &palette->helpLineBg);
	else if (!strcmp(name, "roottext"))
	    updateColorset(fg, bg, &palette->rootTextFg, &palette->rootTextBg);
	else if (!strcmp(name, "emptyscale"))
	    updateColorset(fg, bg, NULL, &palette->emptyScale);
	else if (!strcmp(name, "fullscale"))
	    updateColorset(fg, bg, NULL, &palette->fullScale);
	else if (!strcmp(name, "disentry"))
	    updateColorset(fg, bg, &palette->disabledEntryFg, &palette->disabledEntryBg);
	else if (!strcmp(name, "compactbutton"))
	    updateColorset(fg, bg, &palette->compactButtonFg, &palette->compactButtonBg);
	else if (!strcmp(name, "actsellistbox"))
	    updateColorset(fg, bg, &palette->actSelListboxFg, &palette->actSelListboxBg);
	else if (!strcmp(name, "sellistbox"))
	    updateColorset(fg, bg, &palette->selListboxFg, &palette->selListboxBg);
    }
}

static void initColors(void)
{
    char *colors, *colors_file, buf[16384];
    FILE *f;
    struct newtColors palette;

    palette = newtDefaultColorPalette;

    colors_file = getenv("NEWT_COLORS_FILE");
#ifdef NEWT_COLORS_FILE
    if (colors_file == NULL)
	colors_file = NEWT_COLORS_FILE;
#endif

    if ((colors = getenv("NEWT_COLORS"))) {
	strncpy(buf, colors, sizeof (buf));
	buf[sizeof (buf) - 1] = '\0';
	parseColors(buf, &palette);
    } else if (colors_file && *colors_file && (f = fopen(colors_file, "r"))) {
	size_t r;
	if ((r = fread(buf, 1, sizeof (buf) - 1, f)) > 0) {
	    buf[r] = '\0';
	    parseColors(buf, &palette);
	}
	fclose(f);
    }

    newtSetColors(palette);
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
    /* we can't redraw from scratch, just redisplay SLang screen */
    SLtt_get_screen_size();
    /* SLsmg_reinit_smg(); */
    if (redraw) {
        SLsmg_touch_lines(0, SLtt_Screen_Rows);
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
    /* slang doesn't support multibyte encodings except UTF-8,
       avoid character corruption by redrawing the screen */
    if (strstr (lang, ".euc") != NULL)
	trashScreen = 1;

    (void) strlen(ident);

    SLutf8_enable(-1);
    SLtt_get_terminfo();
    SLtt_get_screen_size();

    MonoValue = getenv(MonoEnv);
    if ( MonoValue != NULL )
	SLtt_Use_Ansi_Colors = 0;

    if ((ret = SLsmg_init_smg()) < 0)
	return ret;
    if ((ret = SLang_init_tty(0, 0, 0)) < 0)
	return ret;

    initColors();
    newtCursorOff();
    initKeymap();

    SLsignal_intr(SIGWINCH, handleSigwinch);
    SLang_getkey_intr_hook = getkeyInterruptHook;

    return 0;
}

/**
 * @brief Closedown the newt library, tidying screen.
 * @returns int , 0. (no errors reported)
 */
int newtFinished(void) {
    if (currentWindow) {
	for (; currentWindow >= windowStack; currentWindow--) {
	    free(currentWindow->buffer);
	    free(currentWindow->title);
	}
	currentWindow = NULL;
    }

    if (currentHelpline) {
	for (; currentHelpline >= helplineStack; currentHelpline--)
	    free(*currentHelpline);
	currentHelpline = NULL;
    }

    freeKeymap();

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

void newtSetColor(int colorset, char *fg, char *bg) {
    if (colorset < NEWT_COLORSET_ROOT ||
        (colorset > NEWT_COLORSET_SELLISTBOX && colorset < NEWT_COLORSET_CUSTOM(0)) ||
	    !SLtt_Use_Ansi_Colors)
	return;

    SLtt_set_color(colorset, "", fg, bg);
}

/* Keymap handling - rewritten by Henning Makholm <henning@makholm.net>,
 * November 2003.
 */

struct kmap_trie_entry {
    char alloced; /* alloced/not first element in array */
    char c ;   /* character got from terminal */
    int code;  /* newt key, or 0 if c does not make a complete sequence */
    struct kmap_trie_entry *contseq; /* sub-trie for character following c */
    struct kmap_trie_entry *next;    /* try this if char received != c */
};

static struct kmap_trie_entry *kmap_trie_root = NULL;
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
        dumpkeys_recursive(kmap_trie_root, 0, f);
        fclose(f);
    }
}
#endif

/* newtBindKey may overwrite a binding that is there already */
static void newtBindKey(char *keyseq, int meaning) {
    struct kmap_trie_entry *root = kmap_trie_root ;
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
	    fresh->alloced = 1;
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
		(*fromcopy)->alloced = 1;
                (*fromcopy)->next = 0 ;
            }
        }
    }
}

int newtGetKey(void) {
    int key, lastcode, errors = 0;
    unsigned char *chptr = keyreader_buf, *lastmatch;
    struct kmap_trie_entry *curr = kmap_trie_root;

    do {
	key = getkey();
	if (key == SLANG_GETKEY_ERROR) {
	    if (needResize) {
                needResize = 0;
		return NEWT_KEY_RESIZE;
            }

	    /* Ignore other signals, but assume that stdin disappeared (the
	     * parent terminal was proably closed) if the error persists.
	     */
	    if (errors++ > 10)
		return NEWT_KEY_ERROR;

	    continue;
	}

	if (key == NEWT_KEY_SUSPEND && suspendCallback)
	    suspendCallback(suspendCallbackData);
    } while (key == NEWT_KEY_SUSPEND || key == SLANG_GETKEY_ERROR);

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
 * @param left. int Size; _not_ including border
 * @param top: int size, _not_ including border
 * @param width unsigned int
 * @param height unsigned int
 * @param title - title string
 * @return zero on success
 */
int newtOpenWindow(int left, int top, 
                   unsigned int width, unsigned int height,
			  const char * title) {
    int j, row, col;
    int n;
    int i;

    newtFlushInput();

    if (currentWindow && currentWindow - windowStack + 1
	    >= sizeof (windowStack) / sizeof (struct Window))
	return 1;

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

    currentWindow->buffer = malloc(sizeof(SLsmg_Char_Type) * (width + 5) * (height + 3));

    row = top - 1;
    col = left - 2;
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
		       currentWindow->width + 5);
	n += currentWindow->width + 5;
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
	SLsmg_write_string((char *)currentWindow->title);
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
 * @returns zero on success
 */
int newtCenteredWindow(unsigned int width,unsigned int height, 
                       const char * title) {
    int top, left;

    top = (int)(SLtt_Screen_Rows - height) / 2;

    /* I don't know why, but this seems to look better */
    if ((SLtt_Screen_Rows % 2) && (top % 2)) top--;

    left = (int)(SLtt_Screen_Cols - width) / 2;

    return newtOpenWindow(left, top, width, height, title);
}

/**
 * @brief Remove the top window
 */
void newtPopWindow(void) {
    newtPopWindowNoRefresh();
    newtRefresh();
}

void newtPopWindowNoRefresh(void) {
    int j, row, col;
    int n = 0;

    if (currentWindow == NULL)
	return;

    row = col = 0;

    row = currentWindow->top - 1;
    col = currentWindow->left - 2;
    if (row < 0)
	row = 0;
    if (col < 0)
	col = 0;
    for (j = 0; j < currentWindow->height + 3; j++, row++) {
	SLsmg_gotorc(row, col);
	SLsmg_write_raw(currentWindow->buffer + n,
			currentWindow->width + 5);
	n += currentWindow->width + 5;
    }

    free(currentWindow->buffer);
    free(currentWindow->title);

    if (currentWindow == windowStack)
	currentWindow = NULL;
    else
	currentWindow--;

    SLsmg_set_char_set(0);

    newtTrashScreen();
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

    if (currentWindow) {
	*row -= currentWindow->top;
	*col -= currentWindow->left;
    }
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
    struct kmap_trie_entry *kmap_trie_escBrack, *kmap_trie_escO;

    /* Here are some entries that will help in handling esc O foo and
       esc [ foo as variants of each other. */
    kmap_trie_root = calloc(3, sizeof (struct kmap_trie_entry));
    kmap_trie_escBrack = kmap_trie_root + 1;
    kmap_trie_escO = kmap_trie_root + 2;

    kmap_trie_root->alloced = 1;
    kmap_trie_root->c = '\033';
    kmap_trie_root->contseq = kmap_trie_escBrack;

    kmap_trie_escBrack->c = '[';
    kmap_trie_escBrack->next = kmap_trie_escO;

    kmap_trie_escO->c = 'O';

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
    kmap_trie_fallback(kmap_trie_escO->contseq, &kmap_trie_escBrack->contseq);
    kmap_trie_fallback(kmap_trie_escBrack->contseq, &kmap_trie_escO->contseq);
}

static void free_keys(struct kmap_trie_entry *kmap, struct kmap_trie_entry *parent, int prepare) {
    if (kmap == NULL)
	return;

    free_keys(kmap->contseq, kmap, prepare);
    free_keys(kmap->next, kmap, prepare);

    if (!kmap->alloced && kmap - parent == 1)
	    return;

    /* find first element in array */
    while (!kmap->alloced)
	kmap--;

    kmap->alloced += prepare ? 1 : -1;
    if (!prepare && kmap->alloced == 1)
	free(kmap);
}

static void freeKeymap() {
    free_keys(kmap_trie_root, NULL, 1);
    free_keys(kmap_trie_root, NULL, 0);
    kmap_trie_root = NULL;
}

/**
 * @brief Delay for a specified number of usecs
 * @param int - number of usecs to wait for.
 */
void newtDelay(unsigned int usecs) {
    usleep(usecs);
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
    SLsmg_write_string(buf);
    SLsmg_gotorc(cursorRow, cursorCol);
}

void newtPushHelpLine(const char * text) {
    if (currentHelpline && currentHelpline - helplineStack + 1
	    >= sizeof (helplineStack) / sizeof (char *))
	return;

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
    SLsmg_write_string((char *)text);
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
	SLsmg_touch_lines(0, SLtt_Screen_Rows);
}
     
void newtComponentGetPosition(newtComponent co, int * left, int * top) {
    if (left) *left = co->left;
    if (top) *top = co->top;
}

void newtComponentGetSize(newtComponent co, int * width, int * height) {
    if (width) *width = co->width;
    if (height) *height = co->height;
}
