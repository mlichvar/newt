/* simple dialog boxes, used by both whiptail and tcl dialog bindings */

#include "config.h"
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <wchar.h>
#include <slang.h>

#include "nls.h"
#include "dialogboxes.h"
#include "newt.h"
#include "newt_pr.h"
#include "popt.h"

#define MAXBUF    200
#define MAXFORMAT 20
#define BUTTONS   4

/* globals -- ick */
static int buttonHeight = 1;
static const char * buttonText[BUTTONS];

int max (int a, int b)
{
	return (a > b) ? a : b;
}

int min (int a, int b)
{
	return ( a < b) ? a : b ;
}
 
static newtComponent (*makeButton)(int left, int right, const char * text) = 
		newtCompactButton;

static const char * getButtonText(int button) {
    const char * text;
    if (button < 0 || button >= BUTTONS)
	return NULL;

    text = buttonText[button];
    if (text)
	return text;

    switch (button) {
	case 0: return dgettext(PACKAGE, "Ok");
	case 1: return dgettext(PACKAGE, "Cancel");
	case 2: return dgettext(PACKAGE, "Yes");
	case 3: return dgettext(PACKAGE, "No");
	default:
		return NULL;
    }
}

static void addButtons(int height, int width, newtComponent form, 
		       newtComponent * okay, newtComponent * cancel, 
		       int flags) {
	// FIXME: DO SOMETHING ABOUT THE HARD-CODED CONSTANTS
    if (flags & FLAG_NOCANCEL) {
	   *okay = makeButton((width - 8) / 2, height - buttonHeight - 1,
			      getButtonText(BUTTON_OK));
	    *cancel = NULL;
	newtFormAddComponent(form, *okay);
    } else {
	*okay = makeButton((width - 18) / 3, height - buttonHeight - 1, 
			   getButtonText(BUTTON_OK));
	*cancel = makeButton(((width - 18) / 3) * 2 + 9, 
				height - buttonHeight - 1, 
				getButtonText(BUTTON_CANCEL));
	newtFormAddComponents(form, *okay, *cancel, NULL);
    }
}

static void cleanNewlines(char *text)
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

static newtComponent textbox(int maxHeight, int width, const char * text,
			int flags, int * height) {
    newtComponent tb;
    int sFlag = (flags & FLAG_SCROLL_TEXT) ? NEWT_FLAG_SCROLL : 0;
    int i;
    char *buf;

    buf = alloca(strlen(text) + 1);
    strcpy(buf, text);
    cleanNewlines(buf);

    tb = newtTextbox(1, 0, width, maxHeight, NEWT_FLAG_WRAP | sFlag);
    newtTextboxSetText(tb, buf);

    i = newtTextboxGetNumLines(tb);
    if (i < maxHeight) {
	newtTextboxSetHeight(tb, i);
	maxHeight = i;
    }

    *height = maxHeight;

    return tb;
}

int gauge(const char * text, int height, int width, poptContext optCon, int fd, 
		int flags) {
    newtComponent form, scale, tb;
    int top;
    const char * arg;
    char * end;
    int val;
    FILE * f = fdopen(fd, "r");
    char buf[3000];
    char buf3[50];
    int i;

    setlinebuf(f);

    if (!(arg = poptGetArg(optCon))) return DLG_ERROR;
    val = strtoul(arg, &end, 10);
    if (*end) return DLG_ERROR;

    tb = textbox(height - 3, width - 2, text, flags, &top);

    form = newtForm(NULL, NULL, 0);

    scale = newtScale(2, height - 2, width - 4, 100);
    newtScaleSet(scale, val);

    newtFormAddComponents(form, tb, scale, NULL);

    newtDrawForm(form);
    newtRefresh();

    do {
	if (!fgets(buf, sizeof(buf) - 1, f))
	    continue;
	buf[strlen(buf) - 1] = '\0';

	if (!strcmp(buf, "XXX")) {
	    while (!fgets(buf3, sizeof(buf3) - 1, f) && !feof(f))
		;
	    if (feof(f))
		break;
	    buf3[strlen(buf3) - 1] = '\0';

	    i = 0;
	    do {
		if (!fgets(buf + i, sizeof(buf) - 1 - i, f))
		    continue;
		if (!strcmp(buf + i, "XXX\n")) {
		    *(buf + i) = '\0';
		    break;
		}
		i = strlen(buf);
	    } while (!feof(f));

	    if (i > 0)
		buf[strlen(buf) - 1] = '\0';
	    else
		buf[0] = '\0';

	    cleanNewlines(buf);
	    newtTextboxSetText(tb, buf);

	    arg = buf3;
	} else {
	    arg = buf;
	}

	val = strtoul(arg, &end, 10);
	if (!*end) {
	    newtScaleSet(scale, val);
	    newtDrawForm(form);
	    newtRefresh();
	}
    } while (!feof(f));

    newtFormDestroy(form);

    return DLG_OKAY;
}

int inputBox(const char * text, int height, int width, poptContext optCon, 
		int flags, char ** result) {
    newtComponent form, entry, okay, cancel, answer, tb;
    const char * val;
    int pFlag = (flags & FLAG_PASSWORD) ? NEWT_FLAG_PASSWORD : 0;
    int rc = DLG_OKAY;
    int top;

    val = poptGetArg(optCon);
    tb = textbox(height - 3 - buttonHeight, width - 2,
			text, flags, &top);

    form = newtForm(NULL, NULL, 0);
    entry = newtEntry(1, top + 1, val, width - 2, &val, 
			NEWT_FLAG_SCROLL | NEWT_FLAG_RETURNEXIT | pFlag);

    newtFormAddComponents(form, tb, entry, NULL);

    addButtons(height, width, form, &okay, &cancel, flags);

    answer = newtRunForm(form);
    *result = NULL;
    if (answer == cancel)
	rc = DLG_CANCEL;
    else if (answer == NULL)
	rc = DLG_ESCAPE;
    else
	*result = strdup(val);

    newtFormDestroy(form);

    return rc;
}

static int mystrncpyw(char *dest, const char *src, int n, int *maxwidth)
{
    int i = 0;
    int w = 0, cw;
    wchar_t c;
    mbstate_t ps;
    const char *p = src;
    char *d = dest;
    
    memset(&ps, 0, sizeof(ps));
    
    for (;;) {
	int ret = mbrtowc(&c, p, MB_CUR_MAX, &ps);
	if (ret <= 0) break;
	if (ret + i >= n) break;
	cw = wcwidth(c);
	if (cw < 0) break;
	if (cw + w > *maxwidth) break;
	w += cw;
	memcpy(d, p, ret);
	d += ret;
	p += ret;
	i += ret;
    }
    dest[i] = '\0';
    *maxwidth = w;
    return i;
}

int listBox(const char * text, int height, int width, poptContext optCon,
		int flags, const char *default_item, char ** result) {
    newtComponent form, okay, tb, answer, listBox;
    newtComponent cancel = NULL;
    const char * arg;
    char * end;
    int listHeight;
    int numItems = 0;
    int allocedItems = 5;
    int i, top;
    int rc = DLG_OKAY;
    char buf[MAXBUF];
    int maxTagWidth = 0;
    int maxTextWidth = 0;
    int defItem = -1;
    int scrollFlag;
    int lineWidth, textWidth, tagWidth;
    struct {
	const char * text;
	const char * tag;
    } * itemInfo = malloc(allocedItems * sizeof(*itemInfo));

    if (itemInfo == NULL) return DLG_ERROR;
    if (!(arg = poptGetArg(optCon))) return DLG_ERROR;
    listHeight = strtoul(arg, &end, 10);
    if (*end) return DLG_ERROR;

    while ((arg = poptGetArg(optCon))) {
	if (allocedItems == numItems) {
	    allocedItems += 5;
	    itemInfo = realloc(itemInfo, sizeof(*itemInfo) * allocedItems);
	    if (itemInfo == NULL) return DLG_ERROR;
	}

	itemInfo[numItems].tag = arg;
	if (default_item && (strcmp(default_item, arg) == 0)) {
	 	defItem = numItems;
	}
	if (!(arg = poptGetArg(optCon))) return DLG_ERROR;

	if (!(flags & FLAG_NOITEM)) {
	    itemInfo[numItems].text = arg;
	} else
	    itemInfo[numItems].text = "";

	if (wstrlen(itemInfo[numItems].text,-1) > (unsigned int)maxTextWidth)
	    maxTextWidth = wstrlen(itemInfo[numItems].text,-1);
	if (wstrlen(itemInfo[numItems].tag,-1) > (unsigned int)maxTagWidth)
	    maxTagWidth = wstrlen(itemInfo[numItems].tag,-1);

	numItems++;
    }
    if (numItems == 0)
	return DLG_ERROR;

    if (flags & FLAG_NOTAGS) {
	    maxTagWidth = 0;
    }

    form = newtForm(NULL, NULL, 0);

    tb = textbox(height - 4 - buttonHeight - listHeight, width - 2,
			text, flags, &top);

    if (listHeight >= numItems) {
	scrollFlag = 0;
	i = 0;
    } else {
	scrollFlag = NEWT_FLAG_SCROLL;
	i = 2;
    }

    lineWidth = min(maxTagWidth + maxTextWidth + i, SLtt_Screen_Cols - 10);
    listBox = newtListbox( (width - lineWidth) / 2 , top + 1, listHeight,
                          NEWT_FLAG_RETURNEXIT | scrollFlag);

    textWidth = maxTextWidth;
    tagWidth = maxTagWidth;
    if (maxTextWidth == 0) {
        tagWidth = lineWidth;
    } else {
       if (maxTextWidth + maxTagWidth + i > lineWidth)
               tagWidth = textWidth = (lineWidth / 2) - 2;
       else {
               tagWidth++;
               textWidth++;
       }
    }

    if (!(flags & FLAG_NOTAGS)) {
       for (i = 0; i < numItems; i++) {
	   int w = tagWidth;
	   int len, j;
	   len = mystrncpyw(buf, itemInfo[i].tag, MAXBUF, &w);
	   for (j = 0; j < tagWidth - w; j++) {
		   if (len + 1 >= MAXBUF)
		       break;
		   buf[len++] = ' ';
	   }
	   buf[len] = '\0';
	   w = textWidth;
	   mystrncpyw(buf + len, itemInfo[i].text, MAXBUF-len, &w);
           newtListboxAddEntry(listBox, buf, (void *)(long) i);
       }
     } else {
        for (i = 0; i < numItems; i++) {
           snprintf(buf, MAXBUF, "%s", itemInfo[i].text);
           newtListboxAddEntry(listBox, buf, (void *)(long) i);
      }
   }

    if (defItem != -1)
	newtListboxSetCurrent (listBox, defItem);

    newtFormAddComponents(form, tb, listBox, NULL);

    addButtons(height, width, form, &okay, &cancel, flags);

    answer = newtRunForm(form);
    *result = NULL;
    if (answer == cancel)
	rc = DLG_CANCEL;
    else if (answer == NULL)
	rc = DLG_ESCAPE;
    else {
	i = (long) newtListboxGetCurrent(listBox);
	*result = strdup(itemInfo[i].tag);
    }

    newtFormDestroy(form);
    free(itemInfo);

    return rc;
}

int checkList(const char * text, int height, int width, poptContext optCon,
		int useRadio, int flags, char *** selections) {
    newtComponent form, okay, tb, subform, answer;
    newtComponent sb = NULL, cancel = NULL;
    const char * arg;
    char * end;
    int listHeight;
    int numBoxes = 0;
    int allocedBoxes = 5;
    int i;
    int numSelected;
    int rc = DLG_OKAY;
    char buf[MAXBUF], format[MAXFORMAT];
    int maxWidth = 0;
    int top;
    struct {
	const char * text;
	const char * tag;
	newtComponent comp;
    } * cbInfo = malloc(allocedBoxes * sizeof(*cbInfo));
    char * cbStates = malloc(allocedBoxes * sizeof(*cbStates));

    if ( (cbInfo == NULL) || (cbStates == NULL)) return DLG_ERROR;
    if (!(arg = poptGetArg(optCon))) return DLG_ERROR;
    listHeight = strtoul(arg, &end, 10);
    if (*end) return DLG_ERROR;

    while ((arg = poptGetArg(optCon))) {
	if (allocedBoxes == numBoxes) {
	    allocedBoxes += 5;
	    cbInfo = realloc(cbInfo, sizeof(*cbInfo) * allocedBoxes);
	    cbStates = realloc(cbStates, sizeof(*cbStates) * allocedBoxes);
	    if ((cbInfo == NULL) || (cbStates == NULL)) return DLG_ERROR;
	}

	cbInfo[numBoxes].tag = arg;
	if (!(arg = poptGetArg(optCon))) return DLG_ERROR;

	if (!(flags & FLAG_NOITEM)) {
	    cbInfo[numBoxes].text = arg;
	    if (!(arg = poptGetArg(optCon))) return DLG_ERROR;
	} else
	    cbInfo[numBoxes].text = "";

	if (!strcmp(arg, "1") || !strcasecmp(arg, "on") || 
		!strcasecmp(arg, "yes"))
	    cbStates[numBoxes] = '*';
	else
	    cbStates[numBoxes] = ' ';

	if (wstrlen(cbInfo[numBoxes].tag,-1) > (unsigned int)maxWidth)
	    maxWidth = wstrlen(cbInfo[numBoxes].tag,-1);

	numBoxes++;
    }

    form = newtForm(NULL, NULL, 0);

    tb = textbox(height - 3 - buttonHeight - listHeight, width - 2,
			text, flags, &top);

    if (listHeight < numBoxes) { 
	sb = newtVerticalScrollbar(width - 4, 
				   top + 1,
				   listHeight, NEWT_COLORSET_CHECKBOX,
				   NEWT_COLORSET_ACTCHECKBOX);
	newtFormAddComponent(form, sb);
    }
    subform = newtForm(sb, NULL, 0);
    newtFormSetBackground(subform, NEWT_COLORSET_CHECKBOX);

    snprintf(format, MAXFORMAT, "%%-%ds  %%s", maxWidth);
    for (i = 0; i < numBoxes; i++) {
	snprintf(buf, MAXBUF, format, cbInfo[i].tag, cbInfo[i].text);

	if (useRadio)
	    cbInfo[i].comp = newtRadiobutton(4, top + 1 + i, buf,
					cbStates[i] != ' ', 
					i ? cbInfo[i - 1].comp : NULL);
	else
	    cbInfo[i].comp = newtCheckbox(4, top + 1 + i, buf,
			      cbStates[i], NULL, cbStates + i);

	newtCheckboxSetFlags(cbInfo[i].comp, NEWT_FLAG_RETURNEXIT, NEWT_FLAGS_SET);
	newtFormAddComponent(subform, cbInfo[i].comp);
    }

    newtFormSetHeight(subform, listHeight);
    newtFormSetWidth(subform, width - 10);

    newtFormAddComponents(form, tb, subform, NULL);

    addButtons(height, width, form, &okay, &cancel, flags);

    answer = newtRunForm(form);
    *selections = NULL;
    if (answer == cancel)
	rc = DLG_CANCEL;
    else if (answer == NULL)
	rc = DLG_ESCAPE;
    else {
	if (useRadio) {
	    answer = newtRadioGetCurrent(cbInfo[0].comp);
	    *selections = malloc(sizeof(char *) * 2);
	    if (*selections == NULL)
		return DLG_ERROR;
	    (*selections)[0] = (*selections)[1] = NULL;
	    for (i = 0; i < numBoxes; i++)
		if (cbInfo[i].comp == answer) {
		    (*selections)[0] = strdup(cbInfo[i].tag);
		    break;
		}
	} else {
	    numSelected = 0;
	    for (i = 0; i < numBoxes; i++) {
		if (cbStates[i] != ' ') numSelected++;
	    }

	    *selections = malloc(sizeof(char *) * (numSelected + 1));
	    if (*selections == NULL)
		return DLG_ERROR;

	    numSelected = 0;
	    for (i = 0; i < numBoxes; i++) {
		if (cbStates[i] != ' ')
		    (*selections)[numSelected++] = strdup(cbInfo[i].tag);
	    }

	    (*selections)[numSelected] = NULL;
	}
    }

    free(cbInfo);
    free(cbStates);
    newtFormDestroy(form);

    return rc;
}

int messageBox(const char * text, int height, int width, int type, int flags) {
    newtComponent form, yes, tb, answer;
    newtComponent no = NULL;
    int rc = DLG_OKAY;
    int tFlag = (flags & FLAG_SCROLL_TEXT) ? NEWT_FLAG_SCROLL : 0;

    form = newtForm(NULL, NULL, 0);

    tb = newtTextbox(1, 1, width - 2, height - 3 - buttonHeight, 
			NEWT_FLAG_WRAP | tFlag);
    newtTextboxSetText(tb, text);

    newtFormAddComponent(form, tb);

    switch ( type ) {
    case MSGBOX_INFO:
	break;
    case MSGBOX_MSG:
	// FIXME Do something about the hard-coded constants
	yes = makeButton((width - 8) / 2, height - 1 - buttonHeight, 
			  getButtonText(BUTTON_OK));
	newtFormAddComponent(form, yes);
	break;
    default:
	yes = makeButton((width - 16) / 3, height - 1 - buttonHeight, 
			 getButtonText(BUTTON_YES));
	no = makeButton(((width - 16) / 3) * 2 + 9, height - 1 - buttonHeight, 
		         getButtonText(BUTTON_NO));
	newtFormAddComponents(form, yes, no, NULL);

	if (flags & FLAG_DEFAULT_NO)
	    newtFormSetCurrent(form, no);
    }

    if ( type != MSGBOX_INFO ) {
	if (newtRunForm(form) == NULL)
		rc = DLG_ESCAPE;

	answer = newtFormGetCurrent(form);

	if (answer == no)
	    rc = DLG_CANCEL;
    }
    else {
	newtDrawForm(form);
	newtRefresh();
    }

    newtFormDestroy(form);

    return rc;
}

void useFullButtons(int state) {
    if (state) {
	buttonHeight = 3;
	makeButton = newtButton;
   } else {
	buttonHeight = 1;
	makeButton = newtCompactButton;
   }
}

void setButtonText(const char * text, int button) {
    if (button < 0 || button >= BUTTONS)
	return;
    buttonText[button] = text;
}
