#include "config.h"
#include <fcntl.h>
#include <popt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <wchar.h>
#include <slang.h>
#include <sys/stat.h>

#include "nls.h"
#include "dialogboxes.h"
#include "newt.h"
#include "newt_pr.h"

enum { NO_ERROR = 0, WAS_ERROR = 1 };

enum mode { MODE_NONE, MODE_INFOBOX, MODE_MSGBOX, MODE_YESNO, MODE_CHECKLIST,
		MODE_INPUTBOX, MODE_RADIOLIST, MODE_MENU, MODE_GAUGE ,
		MODE_TEXTBOX, MODE_PASSWORDBOX};

#define OPT_MSGBOX 		1000
#define OPT_CHECKLIST 		1001
#define OPT_YESNO 		1002
#define OPT_INPUTBOX 		1003
#define OPT_FULLBUTTONS 	1004
#define OPT_MENU	 	1005
#define OPT_RADIOLIST	 	1006
#define OPT_GAUGE	 	1007
#define OPT_INFOBOX	 	1008
#define OPT_TEXTBOX		1009
#define OPT_PASSWORDBOX		1010

static void usage(int err) {
    newtFinished();
    fprintf (err ? stderr : stdout,
             _("Box options: \n"
	       "\t--msgbox <text> <height> <width>\n"
	       "\t--yesno  <text> <height> <width>\n"
	       "\t--infobox <text> <height> <width>\n"
	       "\t--inputbox <text> <height> <width> [init] \n"
	       "\t--passwordbox <text> <height> <width> [init] \n"
	       "\t--textbox <file> <height> <width>\n"
	       "\t--menu <text> <height> <width> <listheight> [tag item] ...\n"
	       "\t--checklist <text> <height> <width> <listheight> [tag item status]...\n"
	       "\t--radiolist <text> <height> <width> <listheight> [tag item status]...\n"
	       "\t--gauge <text> <height> <width> <percent>\n"
	       "Options: (depend on box-option)\n"
	       "\t--clear				clear screen on exit\n"
	       "\t--defaultno			default no button\n"	
	       "\t--default-item <string>		set default string\n"
	       "\t--fb, --fullbuttons		use full buttons\n"
	       "\t--nocancel			no cancel button\n"
	       "\t--yes-button <text>		set text of yes button\n"
	       "\t--no-button <text>		set text of no button\n"
	       "\t--ok-button <text>		set text of ok button\n"
	       "\t--cancel-button <text>		set text of cancel button\n"
	       "\t--noitem			don't display items\n"
	       "\t--notags			don't display tags\n"
	       "\t--separate-output		output one line at a time\n"
	       "\t--output-fd <fd>		output to fd, not stdout\n"
	       "\t--title <title>			display title\n"
	       "\t--backtitle <backtitle>		display backtitle\n"
	       "\t--scrolltext			force vertical scrollbars\n"
	       "\t--topleft			put window in top-left corner\n"
	       "\t-h, --help			print this message\n"
	       "\t-v, --version			print version information\n\n"));
    exit(err ? DLG_ERROR : 0 );
}

static void print_version(void) {
    fprintf (stdout, _("whiptail (newt): %s\n"), VERSION);
}
	     
#if 0
/* FIXME Copied from newt.c
 * Place somewhere better -- dialogboxes? -- amck
 */
int wstrlen(const char *str, int len) {
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
#endif

/*
 * The value of *width is increased if it is not as large as the width of
 * the line.
 */
static const char * lineWidth(int * width, const char * line, int *chrs)
{
    const char *    s = line;

    if ( line == NULL )
       return 0;

   while ( *s != '\0' && *s != '\n' )
       s++;

    if ( *s == '\n' )
       s++;

    *chrs = _newt_wstrlen (line, s - line );
    *width = max(*width, *chrs);

    return s;
}


/*
 * cleanNewlines
 * Handle newlines in text. Hack.
 */
void cleanNewlines (char *text)
{
    char *p, *q;

    for (p = q = text; *p; p++, q++)
        if (*p == '\\' && p[1] == 'n') {
            p++;
            *q = '\n';
        } else
            *q = *p;
    *q = '\0';
}

/*
 * The height of a text string is added to height, and width is increased
 * if it is not big enough to store the text string.
 */
static const char * textSize(int * height, int * width,
                            int maxWidth,
                            const char * text)
{
    int h = 0;
    int w = 0;
    int chrs = 0;


    if ( text == NULL )
       return 0;

   while ( *text != '\0' ) {
       h++;
       text = lineWidth(width, text, &chrs);
       /* Allow for text overflowing. May overestimate a bit */
       h += chrs /  maxWidth;
    }

    h += 2;
   w += 2;

    *height += h;
    *width += w;

    *width = min(*width, maxWidth);
    return text;
}

/*
 * Add space for buttons.
 * NOTE: when this is internationalized, the button width might change.
 */
static void spaceForButtons(int * height, int * width, int count, int full) {
    /* Make space for the buttons */
    if ( full ) {
       *height += 4;
       if ( count == 1 )
           *width = max(*width, 7);
       else
           *width = max(*width, 20);
    }
    else {
       *height += 2;
       if ( count == 1 )
           *width = max(*width, 7);
       else
           *width = max(*width, 19);
    }
}

static int menuSize(int * height, int * width, enum mode mode,
		    poptContext options) {
    const char ** argv = poptGetArgs(options);
    const char ** items = argv;
    int         h = 0;
    int         tagWidth = 0;
    int         descriptionWidth = 0;
    int         overhead = 10;
    static char buf[20];

    if ( argv == 0 || *argv == 0 )
       return 0;

    argv++;
    if ( mode == MODE_MENU )
       overhead = 5;

    while ( argv[0] != 0 && argv[1] ) {
       tagWidth = max(tagWidth, strlen(argv[0]));
       descriptionWidth = max(descriptionWidth, strlen(argv[1]));

       if ( mode == MODE_MENU )
           argv += 2;
       else
          argv += 3;
       h++;
    }

    *width = max(*width, tagWidth + descriptionWidth + overhead);
    *width = min(*width, SLtt_Screen_Cols);

    h = min(h, SLtt_Screen_Rows - *height - 4);
    *height = *height + h + 1;
    sprintf(buf, "%d", h);
   *items = buf;
    return 0;
}

/*
 * Guess the size of a window, given what will be displayed within it.
 */
static void guessSize(int * height, int * width, enum mode mode,
                     int * flags, int fullButtons,
                     const char * title, const char * text,
                     poptContext options) {

    int w = 0, h = 0, chrs = 0;

    textSize(&h, &w, SLtt_Screen_Cols -4 , text);     /* Width and height for text */
    lineWidth(&w, title, &chrs);             /* Width for title */

    if ( w > 0 )
       w += 4;

    switch ( mode ) {
       case MODE_CHECKLIST:
       case MODE_RADIOLIST:
       case MODE_MENU:
           spaceForButtons(&h, &w, *flags & FLAG_NOCANCEL ? 1 : 2,
            fullButtons);
           menuSize(&h, &w, mode, options);
               break;
       case MODE_YESNO:
       case MODE_MSGBOX:
           spaceForButtons(&h, &w, 1, fullButtons);
           break;
       case MODE_INPUTBOX:
           spaceForButtons(&h, &w, *flags & FLAG_NOCANCEL ? 1 : 2,
            fullButtons);
           h += 1;
           break;
       case MODE_GAUGE:
           h += 2;
           break;
       case MODE_NONE:
	   break;
       default:
               break;
    };

    /*
     * Fixed window-border overhead.
     * NOTE: This will change if we add a way to turn off drop-shadow and/or
     * box borders. That would be desirable for display-sized screens.
     */
    w += 2;
    h += 2;

   if ( h > SLtt_Screen_Rows - 1 ) {
       h = SLtt_Screen_Rows - 1;
       *flags |= FLAG_SCROLL_TEXT;
       w += 2; /* Add width of slider - is this right? */
    }

    *width = min(max(*width, w), SLtt_Screen_Cols);
    *height = max(*height, h);
}

char *
readTextFile(const char * filename)
{
    int fd = open(filename, O_RDONLY, 0);
    struct stat s;
    char * buf;

    if ( fd < 0 || fstat(fd, &s) != 0 ) {
       perror(filename);
       exit(DLG_ERROR);
     }

    if ( (buf = malloc(s.st_size + 1)) == 0 ) {
       fprintf(stderr, _("%s: too large to display.\n"), filename);
       exit(DLG_ERROR);
    }

    if ( read(fd, buf, s.st_size) != s.st_size ) {
        perror(filename);
        exit(DLG_ERROR);
    }
   close(fd);
   buf[s.st_size] = '\0';
   return buf;
}

int main(int argc, const char ** argv) {
    enum mode mode = MODE_NONE;
    poptContext optCon;
    int arg;
    char * text;
    const char * nextArg;
    char * end;
    int height;
    int width;
    int fd = -1;
    int needSpace = 0;
    int noCancel = 0;
    int noTags = 0;
    int noItem = 0;
    int clear = 0;
    int scrollText = 0;
    int rc = DLG_CANCEL;
    int flags = 0;
    int defaultNo = 0;
    int separateOutput = 0;
    int fullButtons = 0;
    int outputfd = 2;
    int topLeft = 0;
    FILE *output = stderr;
    char * result;
    char ** selections, ** next;
    char * title = NULL;
    char *default_item = NULL;
    char * backtitle = NULL;
    char * yes_button = NULL;
    char * no_button = NULL;
    char * ok_button = NULL;
    char * cancel_button = NULL;
    int help = 0, version = 0;
    struct poptOption optionsTable[] = {
	    { "backtitle", '\0', POPT_ARG_STRING, &backtitle, 0 },
	    { "checklist", '\0', 0, 0, OPT_CHECKLIST },
	    { "clear", '\0', 0, &clear, 0 },
	    { "defaultno", '\0', 0, &defaultNo, 0 },
	    { "inputbox", '\0', 0, 0, OPT_INPUTBOX },
	    { "fb", '\0', 0, 0, OPT_FULLBUTTONS },
	    { "fullbuttons", '\0', 0, 0, OPT_FULLBUTTONS },
	    { "gauge", '\0', 0, 0, OPT_GAUGE },
	    { "infobox", '\0', 0, 0, OPT_INFOBOX },
	    { "menu", '\0', 0, 0, OPT_MENU },
	    { "msgbox", '\0', 0, 0, OPT_MSGBOX },
	    { "nocancel", '\0', 0, &noCancel, 0 },
	    { "noitem", '\0', 0, &noItem, 0 },
            { "default-item", '\0', POPT_ARG_STRING, &default_item, 0},
	    { "notags", '\0', 0, &noTags, 0 },
	    { "radiolist", '\0', 0, 0, OPT_RADIOLIST },
	    { "scrolltext", '\0', 0, &scrollText, 0 },
	    { "separate-output", '\0', 0, &separateOutput, 0 },
	    { "title", '\0', POPT_ARG_STRING, &title, 0 },
	    { "textbox", '\0', 0, 0, OPT_TEXTBOX },
	    { "topleft", '\0', 0, &topLeft, 0 },
	    { "yesno", '\0', 0, 0, OPT_YESNO },
	    { "passwordbox", '\0', 0, 0, OPT_PASSWORDBOX },
	    { "output-fd", '\0',  POPT_ARG_INT, &outputfd, 0 },
	    { "yes-button", '\0', POPT_ARG_STRING, &yes_button, 0},
	    { "no-button", '\0', POPT_ARG_STRING, &no_button, 0},
	    { "ok-button", '\0', POPT_ARG_STRING, &ok_button, 0},
	    { "cancel-button", '\0', POPT_ARG_STRING, &cancel_button, 0},
	    { "help", 'h', 0,  &help, 0, NULL, NULL },
	    { "version", 'v', 0, &version, 0, NULL, NULL },
	    { 0, 0, 0, 0, 0 } 
    };
   
#ifdef ENABLE_NLS
    setlocale (LC_ALL, "");
    bindtextdomain (PACKAGE, LOCALEDIR);
    textdomain (PACKAGE);
#endif

    optCon = poptGetContext("whiptail", argc, argv, optionsTable, 0);

    while ((arg = poptGetNextOpt(optCon)) > 0) {
	switch (arg) {
	  case OPT_INFOBOX:
	    if (mode != MODE_NONE) usage(WAS_ERROR);
	    mode = MODE_INFOBOX;
	    break;

	  case OPT_MENU:
	    if (mode != MODE_NONE) usage(WAS_ERROR);
	    mode = MODE_MENU;
	    break;

	  case OPT_MSGBOX:
	    if (mode != MODE_NONE) usage(WAS_ERROR);
	    mode = MODE_MSGBOX;
	    break;

	  case OPT_TEXTBOX:
    	    if (mode != MODE_NONE) usage(WAS_ERROR);
	    mode = MODE_TEXTBOX;
	    break;

	  case OPT_PASSWORDBOX:
	    if (mode != MODE_NONE) usage(WAS_ERROR);
	    mode = MODE_PASSWORDBOX;
	    break;

	  case OPT_RADIOLIST:
	    if (mode != MODE_NONE) usage(WAS_ERROR);
	    mode = MODE_RADIOLIST;
	    break;

	  case OPT_CHECKLIST:
	    if (mode != MODE_NONE) usage(WAS_ERROR);
	    mode = MODE_CHECKLIST;
	    break;

	  case OPT_FULLBUTTONS:
	    fullButtons = 1;
	    useFullButtons(1);
	    break;

	  case OPT_YESNO:
	    if (mode != MODE_NONE) usage(WAS_ERROR);
	    mode = MODE_YESNO;
	    break;

	  case OPT_GAUGE:
	    if (mode != MODE_NONE) usage(WAS_ERROR);
	    mode = MODE_GAUGE;
	    break;

	  case OPT_INPUTBOX:
	    if (mode != MODE_NONE) usage(WAS_ERROR);
	    mode = MODE_INPUTBOX;
	    break;
	}
    }
    
    if (help) {
	    usage(NO_ERROR);
	    exit(0);
    }
    if (version) {
	    print_version();
	    exit(0);
    }
    
    if (arg < -1) {
	fprintf(stderr, "%s: %s\n", 
		poptBadOption(optCon, POPT_BADOPTION_NOALIAS), 
		poptStrerror(arg));
	exit(1);
    }

    output = fdopen (outputfd, "w");
    if (output == NULL ) {
        perror ("Cannot open output-fd\n");
        exit (DLG_ERROR);
    }

    if (mode == MODE_NONE) usage(WAS_ERROR);

    if (!(nextArg = poptGetArg(optCon))) usage(WAS_ERROR);
    text = strdup(nextArg);

    if (mode == MODE_TEXTBOX) {
	char *t = text;
	text = readTextFile(t);
	free(t);
    }

    if (!(nextArg = poptGetArg(optCon))) usage(WAS_ERROR);
    height = strtoul(nextArg, &end, 10);
    if (*end) usage(WAS_ERROR);

    if (!(nextArg = poptGetArg(optCon))) usage(WAS_ERROR);
    width = strtoul(nextArg, &end, 10);
    if (*end) usage(WAS_ERROR);

    if (mode == MODE_GAUGE) {
	fd = dup(0);
	if (fd < 0 || close(0) < 0) {
	    perror("dup/close stdin");
	    exit(DLG_ERROR);
	}
	if (open("/dev/tty", O_RDWR) != 0) {
	    perror("open /dev/tty");
	    exit(DLG_ERROR);
	}
    }

    newtInit();
    newtCls();

    cleanNewlines(text);

    if ( height <= 0 || width <= 0 )
       guessSize(&height, &width, mode, &flags, fullButtons, title, text,
                 optCon);

    width -= 2;
    height -= 2;

    newtOpenWindow(topLeft ? 1 : (SLtt_Screen_Cols - width) / 2,
		   topLeft ? 1 : (SLtt_Screen_Rows - height) / 2, width, height, title);
    if (backtitle)
	newtDrawRootText(0, 0, backtitle);

    if (ok_button)
	setButtonText(ok_button, BUTTON_OK);
    if (cancel_button)
	setButtonText(cancel_button, BUTTON_CANCEL);
    if (yes_button)
	setButtonText(yes_button, BUTTON_YES);
    if (no_button)
	setButtonText(no_button, BUTTON_NO);

    if (noCancel) flags |= FLAG_NOCANCEL;
    if (noItem) flags |= FLAG_NOITEM;
    if (noTags) flags |= FLAG_NOTAGS;
    if (scrollText) flags |= FLAG_SCROLL_TEXT;
    if (defaultNo) flags |= FLAG_DEFAULT_NO;

    switch (mode) {
      case MODE_MSGBOX:
      case MODE_TEXTBOX:
	rc = messageBox(text, height, width, MSGBOX_MSG, flags);
	break;

      case MODE_INFOBOX:
	rc = messageBox(text, height, width, MSGBOX_INFO, flags);
	break;

      case MODE_YESNO:
	rc = messageBox(text, height, width, MSGBOX_YESNO, flags);
	break;

      case MODE_INPUTBOX:
	rc = inputBox(text, height, width, optCon, flags, &result);
	if (rc == DLG_OKAY) {
	    fprintf(output, "%s", result);
	    free(result);
	}
	break;

      case MODE_PASSWORDBOX:
	rc = inputBox(text, height, width, optCon, flags | FLAG_PASSWORD,
	      &result);
	if (rc == DLG_OKAY) {
	    fprintf (output, "%s", result);
	    free(result);
	}
	break;

      case MODE_MENU:
	rc = listBox(text, height, width, optCon, flags, default_item, &result);
	if (rc == DLG_OKAY) {
	    fprintf(output, "%s", result);
	    free(result);
	}
	break;

      case MODE_RADIOLIST:
	rc = checkList(text, height, width, optCon, 1, flags, &selections);
	if (rc == DLG_OKAY && selections[0]) {
	    fprintf(output, "%s", selections[0]);
	    free(selections[0]);
	    free(selections);
	}
	break;

      case MODE_CHECKLIST:
	rc = checkList(text, height, width, optCon, 0, flags, &selections);

	if (!rc) {
	    for (next = selections; *next; next++) {
		if (!separateOutput) {
		    if (needSpace) putc(' ', output);
		    fprintf(output, "\"%s\"", *next);
		    needSpace = 1;
		} else {
		    fprintf(output, "%s\n", *next);
		}
		free(*next);
	    }

	    free(selections);
	}
	break;

      case MODE_GAUGE:
	rc = gauge(text, height, width, optCon, fd, flags);
	break;

      default:
	usage(WAS_ERROR);
    }

    if (rc == DLG_ERROR) usage(WAS_ERROR);

    if (clear)
	newtPopWindow();
    newtFinished();

    free(text);
    poptFreeContext(optCon);

    return ( rc == DLG_ESCAPE ) ? -1 : rc;
}
