/* a reasonable dialog */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "newt.h"
#include "popt.h"

enum mode { MODE_NONE, MODE_MSGBOX, MODE_YESNO, MODE_CHECKLIST, MODE_INPUTBOX,
	    MODE_RADIOLIST, MODE_MENU, MODE_GAUGE };

#define OPT_MSGBOX 		1000
#define OPT_CHECKLIST 		1001
#define OPT_YESNO 		1002
#define OPT_INPUTBOX 		1003
#define OPT_FULLBUTTONS 	1004
#define OPT_MENU	 	1005
#define OPT_RADIOLIST	 	1006
#define OPT_GAUGE	 	1007

#define MSGBOX_MSG 0 
#define MSGBOX_YESNO 1

#define FLAG_NOITEM 		(1 << 0)
#define FLAG_NOCANCEL 		(1 << 1)
#define FLAG_SEPARATE_OUTPUT 	(1 << 2)
#define FLAG_SCROLL_TEXT 	(1 << 3)

static void usage(void) {
    fprintf(stderr, "ndialog: bad parametrs (see man dialog(1) for details)\n");
    exit(1);
}

/* globals -- ick */
int buttonHeight = 1;
newtComponent (*makeButton)(int left, int right, char * text) = 
		newtCompactButton;

void addButtons(int height, int width, newtComponent form, 
		newtComponent * okay, newtComponent * cancel, int flags) {
    if (flags & FLAG_NOCANCEL) {
	*okay = makeButton((width - 8) / 2, height - buttonHeight - 1, "Ok");
	*cancel = NULL;
	newtFormAddComponent(form, *okay);
    } else {
	*okay = makeButton((width - 18) / 3, height - buttonHeight - 1, "Ok");
	*cancel = makeButton(((width - 18) / 3) * 2 + 9, 
				height - buttonHeight - 1, "Cancel");
	newtFormAddComponents(form, *okay, *cancel, NULL);
    }
}

newtComponent textbox(int maxHeight, int width, char * text, int flags, 
			int * height) {
    newtComponent tb;
    int sFlag = (flags & FLAG_SCROLL_TEXT) ? NEWT_TEXTBOX_SCROLL : 0;
    int i;
    char * buf, * src, * dst;

    dst = buf = alloca(strlen(text) + 1);
    src = text; 
    while (*src) {
	if (*src == '\\' && *(src + 1) == 'n') {
	    src += 2;
	    *dst++ = '\n';
	} else
	    *dst++ = *src++;
    }
    *dst++ = '\0';

    tb = newtTextbox(1, 0, width, maxHeight, NEWT_TEXTBOX_WRAP | sFlag);
    newtTextboxSetText(tb, buf);

    i = newtTextboxGetNumLines(tb);
    if (i < maxHeight) {
	newtTextboxSetHeight(tb, i);
	maxHeight = i;
    }

    *height = maxHeight;

    return tb;
}


int gauge(char * text, int height, int width, poptContext optCon, int fd, 
		int flags) {
    newtComponent form, scale, tb;
    int rc;
    int top;
    char * arg, * end;
    int val;
    FILE * f = fdopen(fd, "r");
    char buf[3000];
    char buf3[50];
    int i;

    setlinebuf(f);

    if (!(arg = poptGetArg(optCon))) usage();
    val = strtoul(arg, &end, 10);
    if (*end) usage();

    tb = textbox(height - 3, width - 2, text, flags, &top);

    form = newtForm(NULL, NULL, 0);

    scale = newtScale(2, height - 2, width - 4, 100);
    newtScaleSet(scale, val);

    newtFormAddComponents(form, tb, scale, NULL);

    newtDrawForm(form);
    newtRefresh();

    while (fgets(buf, sizeof(buf) - 1, f)) {
	buf[strlen(buf) - 1] = '\0';

	if (!strcmp(buf, "XXX")) {
	    fgets(buf3, sizeof(buf3) - 1, f);
	    buf3[strlen(buf3) - 1] = '\0';
	    arg = buf3;

	    i = 0;
	    while (fgets(buf + i, sizeof(buf) - 1 - i, f)) {
		buf[strlen(buf) - 1] = '\0';
		if (!strcmp(buf + i, "XXX")) {
		    *(buf + i) = '\0';
		    break;
		}
		i = strlen(buf);
	    }

	    newtTextboxSetText(tb, buf);
 	} else {
	    arg = buf;
	}

	val = strtoul(buf, &end, 10);
	if (!*end) {
	    newtScaleSet(scale, val);
	    newtDrawForm(form);
	    newtRefresh();
	}
    }

    return rc;
}

int inputBox(char * text, int height, int width, poptContext optCon, 
		int flags) {
    newtComponent form, entry, okay, cancel, answer, tb;
    char * val;
    int rc;
    int top;

    val = poptGetArg(optCon);
    tb = textbox(height - 3 - buttonHeight, width - 2,
			text, flags, &top);

    form = newtForm(NULL, NULL, 0);
    entry = newtEntry(1, top + 1, val, width - 2, &val, 
			NEWT_FLAG_SCROLL | NEWT_FLAG_RETURNEXIT);

    newtFormAddComponents(form, tb, entry, NULL);

    addButtons(height, width, form, &okay, &cancel, flags);

    answer = newtRunForm(form);
    if (answer == cancel)
	rc = 1;

    fprintf(stderr, "%s", val);

    return rc;
}

int listBox(char * text, int height, int width, poptContext optCon,
		int flags) {
    newtComponent form, okay, tb, answer, listBox;
    newtComponent cancel = NULL;
    char * arg, * end;
    int listHeight;
    int numItems = 0;
    int allocedItems = 5;
    int i, top;
    int rc = 0;
    char buf[80], format[20];
    int maxTagWidth = 0;
    int maxTextWidth = 0;
    int noScrollFlag;
    struct {
	char * text;
	char * tag;
    } * itemInfo = malloc(allocedItems * sizeof(*itemInfo));

    if (!(arg = poptGetArg(optCon))) usage();
    listHeight = strtoul(arg, &end, 10);
    if (*end) usage();

    while ((arg = poptGetArg(optCon))) {
	if (allocedItems == numItems) {
	    allocedItems += 5;
	    itemInfo = realloc(itemInfo, sizeof(*itemInfo) * allocedItems);
	}

	itemInfo[numItems].tag = arg;
	if (!(arg = poptGetArg(optCon))) usage();

	if (!(flags & FLAG_NOITEM)) {
	    itemInfo[numItems].text = arg;
	} else
	    itemInfo[numItems].text = "";

	if (strlen(itemInfo[numItems].text) > maxTextWidth)
	    maxTextWidth = strlen(itemInfo[numItems].text);
	if (strlen(itemInfo[numItems].tag) > maxTagWidth)
	    maxTagWidth = strlen(itemInfo[numItems].tag);

	numItems++;
    }

    form = newtForm(NULL, NULL, 0);

    tb = textbox(height - 4 - buttonHeight - listHeight, width - 2,
			text, flags, &top);

    if (listHeight >= numItems) {
	noScrollFlag = NEWT_FLAG_NOSCROLL;
	i = 0;
    } else {
	i = 2;
    }

    listBox = newtListbox(3 + ((width - 10 - maxTagWidth - maxTextWidth - i) 
					/ 2),
			  top + 1, listHeight, 
			    NEWT_FLAG_RETURNEXIT | noScrollFlag);

    sprintf(format, "%%-%ds  %%s", maxTagWidth);
    for (i = 0; i < numItems; i++) {
	sprintf(buf, format, itemInfo[i].tag, itemInfo[i].text);
	newtListboxAddEntry(listBox, buf, (void *) i);
    }

    newtFormAddComponents(form, tb, listBox, NULL);

    addButtons(height, width, form, &okay, &cancel, flags);

    answer = newtRunForm(form);
    if (answer == cancel)
	rc = 1;

    i = (int) newtListboxGetCurrent(listBox);
    fprintf(stderr, "%s", itemInfo[i].tag);

    return rc;
}

int checkList(char * text, int height, int width, poptContext optCon,
		int useRadio, int flags) {
    newtComponent form, okay, tb, subform, answer;
    newtComponent sb = NULL, cancel = NULL;
    char * arg, * end;
    int listHeight;
    int numBoxes = 0;
    int allocedBoxes = 5;
    int i;
    int rc = 0;
    char buf[80], format[20];
    int maxWidth = 0;
    int needSpace = 0;
    int top;
    struct {
	char * text;
	char * tag;
	newtComponent comp;
    } * cbInfo = malloc(allocedBoxes * sizeof(*cbInfo));
    char * cbStates = malloc(allocedBoxes * sizeof(cbStates));

    if (!(arg = poptGetArg(optCon))) usage();
    listHeight = strtoul(arg, &end, 10);
    if (*end) usage();

    while ((arg = poptGetArg(optCon))) {
	if (allocedBoxes == numBoxes) {
	    allocedBoxes += 5;
	    cbInfo = realloc(cbInfo, sizeof(*cbInfo) * allocedBoxes);
	    cbStates = realloc(cbStates, sizeof(*cbStates) * allocedBoxes);
	}

	cbInfo[numBoxes].tag = arg;
	if (!(arg = poptGetArg(optCon))) usage();

	if (!(flags & FLAG_NOITEM)) {
	    cbInfo[numBoxes].text = arg;
	    if (!(arg = poptGetArg(optCon))) usage();
	} else
	    cbInfo[numBoxes].text = "";

	if (!strcmp(arg, "1") || !strcasecmp(arg, "on") || 
		!strcasecmp(arg, "yes"))
	    cbStates[numBoxes] = '*';
	else
	    cbStates[numBoxes] = ' ';

	if (strlen(cbInfo[numBoxes].tag) > maxWidth)
	    maxWidth = strlen(cbInfo[numBoxes].tag);

	numBoxes++;
    }

    form = newtForm(NULL, NULL, 0);

    if (listHeight < numBoxes) { 
	sb = newtVerticalScrollbar(width - 4, 
				   height - 2 - buttonHeight - listHeight,
				   listHeight, NEWT_COLORSET_CHECKBOX,
				   NEWT_COLORSET_ACTCHECKBOX);
	newtFormAddComponent(form, sb);
    }
    subform = newtForm(sb, NULL, 0);
    newtFormSetBackground(subform, NEWT_COLORSET_CHECKBOX);

    tb = textbox(height - 4 - buttonHeight - listHeight, width - 2,
			text, flags, &top);

    sprintf(format, "%%-%ds  %%s", maxWidth);
    for (i = 0; i < numBoxes; i++) {
	sprintf(buf, format, cbInfo[i].tag, cbInfo[i].text);

	if (useRadio)
	    cbInfo[i].comp = newtRadiobutton(4, top + 1 + i, buf,
					cbStates[i] != ' ', 
					i ? cbInfo[i - 1].comp : NULL);
	else
	    cbInfo[i].comp = newtCheckbox(4, top + 1 + i, buf,
			      cbStates[i], NULL, cbStates + i);

	newtFormAddComponent(subform, cbInfo[i].comp);
    }

    newtFormSetHeight(subform, listHeight);
    newtFormSetWidth(subform, width - 10);

    newtFormAddComponents(form, tb, subform, NULL);

    addButtons(height, width, form, &okay, &cancel, flags);

    answer = newtRunForm(form);
    if (answer == cancel)
	rc = 1;

    if (useRadio) {
	answer = newtRadioGetCurrent(cbInfo[0].comp);
	for (i = 0; i < numBoxes; i++) 
	    if (cbInfo[i].comp == answer) {
		fprintf(stderr, "%s", cbInfo[i].tag);
		break;
	    }
    } else {
	for (i = 0; i < numBoxes; i++) {
	    /* this should do proper quoting */
	    if (cbStates[i] != ' ') {
		if (!(flags & FLAG_SEPARATE_OUTPUT)) {
		    if (needSpace) putc(' ', stderr);
		    fprintf(stderr, "\"%s\"", cbInfo[i].tag);
		    needSpace = 1;
		} else {
		    fprintf(stderr, "%s\n", cbInfo[i].tag);
		}
	    }
	}
    }

    return rc;
}

int messageBox(char * text, int height, int width, int type, int flags) {
    newtComponent form, yes, tb, answer;
    newtComponent no = NULL;
    int tFlag = (flags & FLAG_SCROLL_TEXT) ? NEWT_TEXTBOX_SCROLL : 0;

    form = newtForm(NULL, NULL, 0);

    tb = newtTextbox(1, 1, width - 2, height - 3 - buttonHeight, 
			NEWT_TEXTBOX_WRAP | tFlag);
    newtTextboxSetText(tb, text);

    newtFormAddComponent(form, tb);

    if (type == MSGBOX_MSG) {
	yes = makeButton((width - 8) / 2, height - 1 - buttonHeight, "Ok");
	newtFormAddComponent(form, yes);
    } else {
	yes = makeButton((width - 16) / 3, height - 1 - buttonHeight, "Yes");
	no = makeButton(((width - 16) / 3) * 2 + 9, height - 1 - buttonHeight, 
			"No");
	newtFormAddComponents(form, yes, no, NULL);
    }

    answer = newtRunForm(form);

    if (answer == no)
	return 1;

    return 0;
}

int main(int argc, char ** argv) {
    enum mode mode = MODE_NONE;
    poptContext optCon;
    int arg;
    char * optArg;
    char * text;
    char * nextArg;
    char * end;
    int height;
    int width;
    int fd;
    int noCancel = 0;
    int noItem = 0;
    int clear = 0;
    int scrollText = 0;
    int rc = 1;
    int flags = 0;
    int separateOutput = 0;
    char * title = NULL;
    char * backtitle = NULL;
    struct poptOption optionsTable[] = {
	    { "backtitle", '\0', POPT_ARG_STRING, &backtitle, 0 },
	    { "checklist", '\0', 0, 0, OPT_CHECKLIST },
	    { "clear", '\0', 0, &clear, 0 },
	    { "inputbox", '\0', 0, 0, OPT_INPUTBOX },
	    { "fb", '\0', 0, 0, OPT_FULLBUTTONS },
	    { "fullbuttons", '\0', 0, 0, OPT_FULLBUTTONS },
	    { "gauge", '\0', 0, 0, OPT_GAUGE },
	    { "menu", '\0', 0, 0, OPT_MENU },
	    { "msgbox", '\0', 0, 0, OPT_MSGBOX },
	    { "nocancel", '\0', 0, &noCancel, 0 },
	    { "noitem", '\0', 0, &noItem, 0 },
	    { "radiolist", '\0', 0, 0, OPT_RADIOLIST },
	    { "scrolltext", '\0', 0, &scrollText, 0 },
	    { "separate-output", '\0', 0, &separateOutput, 0 },
	    { "title", '\0', POPT_ARG_STRING, &title, 0 },
	    { "yesno", '\0', 0, 0, OPT_YESNO },
	    { 0, 0, 0, 0, 0 } 
    };
    
    optCon = poptGetContext("ndialog", argc, argv, optionsTable, 0);

    while ((arg = poptGetNextOpt(optCon)) > 0) {
	optArg = poptGetOptArg(optCon);

	switch (arg) {
	  case OPT_MENU:
	    if (mode != MODE_NONE) usage();
	    mode = MODE_MENU;
	    break;

	  case OPT_MSGBOX:
	    if (mode != MODE_NONE) usage();
	    mode = MODE_MSGBOX;
	    break;

	  case OPT_RADIOLIST:
	    if (mode != MODE_NONE) usage();
	    mode = MODE_RADIOLIST;
	    break;

	  case OPT_CHECKLIST:
	    if (mode != MODE_NONE) usage();
	    mode = MODE_CHECKLIST;
	    break;

	  case OPT_FULLBUTTONS:
	    buttonHeight = 3;
	    makeButton = newtButton;
	    break;

	  case OPT_YESNO:
	    if (mode != MODE_NONE) usage();
	    mode = MODE_YESNO;
	    break;

	  case OPT_GAUGE:
	    if (mode != MODE_NONE) usage();
	    mode = MODE_GAUGE;
	    break;

	  case OPT_INPUTBOX:
	    if (mode != MODE_NONE) usage();
	    mode = MODE_INPUTBOX;
	    break;
	}
    }
    
    if (arg < -1) {
	fprintf(stderr, "%s: %s\n", 
		poptBadOption(optCon, POPT_BADOPTION_NOALIAS), 
		poptStrerror(arg));
	exit(1);
    }

    if (mode == MODE_NONE) usage();

    if (!(text = poptGetArg(optCon))) usage();

    if (!(nextArg = poptGetArg(optCon))) usage();
    height = strtoul(nextArg, &end, 10);
    if (*end) usage();

    if (!(nextArg = poptGetArg(optCon))) usage();
    width = strtoul(nextArg, &end, 10);
    if (*end) usage();

    if (mode == MODE_GAUGE) {
	fd = dup(0);
	close(0);
	if (open("/dev/tty", O_RDWR) != 0) perror("open /dev/tty");
    }

    newtInit();
    newtCls();
    width -= 2;
    height -= 2;
    newtOpenWindow((80 - width) / 2, (24 - height) / 2, width, height, title);

    if (backtitle)
	newtDrawRootText(0, 0, backtitle);

    if (noCancel) flags |= FLAG_NOCANCEL;
    if (noItem) flags |= FLAG_NOITEM;
    if (separateOutput) flags |= FLAG_SEPARATE_OUTPUT;
    if (scrollText) flags |= FLAG_SCROLL_TEXT;

    switch (mode) {
      case MODE_MSGBOX:
	rc = messageBox(text, height, width, MSGBOX_MSG, flags);
	break;

      case MODE_YESNO:
	rc = messageBox(text, height, width, MSGBOX_YESNO, flags);
	break;

      case MODE_INPUTBOX:
	rc = inputBox(text, height, width, optCon, flags);
	break;

      case MODE_MENU:
	rc = listBox(text, height, width, optCon, flags);
	break;

      case MODE_RADIOLIST:
	rc = checkList(text, height, width, optCon, 1, flags);
	break;

      case MODE_CHECKLIST:
	rc = checkList(text, height, width, optCon, 0, flags);
	break;

      case MODE_GAUGE:
	rc = gauge(text, height, width, optCon, fd, flags);
	break;

      default:
	usage();
    }

    if (clear)
	newtPopWindow();
    newtFinished();

    return rc;
}
