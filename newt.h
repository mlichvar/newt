#ifndef H_NEWT
#define H_NEWT

#define COLORSET_ROOT 		2
#define COLORSET_BORDER 	3
#define COLORSET_WINDOW		4
#define COLORSET_SHADOW		5
#define COLORSET_TITLE		6
#define COLORSET_BUTTON		7
#define COLORSET_ACTBUTTON	8
#define COLORSET_CHECKBOX	9
#define COLORSET_ACTCHECKBOX	10
#define COLORSET_ENTRY		11
#define COLORSET_LABEL		12
#define COLORSET_LISTBOX	13
#define COLORSET_ACTLISTBOX	14
#define COLORSET_TEXTBOX	15
#define COLORSET_ACTTEXTBOX	16

struct newtColors {
    char * rootFg, * rootBg;
    char * borderFg, * borderBg;
    char * windowFg, * windowBg;
    char * shadowFg, * shadowBg;
    char * titleFg, * titleBg;
    char * buttonFg, * buttonBg;
    char * actButtonFg, * actButtonBg;
    char * checkboxFg, * checkboxBg;
    char * actCheckboxFg, * actCheckboxBg;
    char * entryFg, * entryBg;
    char * labelFg, * labelBg;
    char * listboxFg, * listboxBg;
    char * actListboxFg, * actListboxBg;
    char * textboxFg, * textboxBg;
    char * actTextboxFg, * actTextboxBg;
};

typedef struct newtComponent * newtComponent;


extern struct newtColors newtDefaultColorPalette;

typedef void (*newtCallback)(newtComponent, void *);

int newtInit(void);
int newtFinished(void);
void newtCls(void);
void newtWaitForKey(void);
void newtClearKeyBuffer(void);
void newtDelay(int usecs);
/* top, left are *not* counting the border */
int newtOpenWindow(int left, int top, int width, int height, 
			  char * title);
void newtPopWindow(void);
void newtSetColors(struct newtColors colors);
void newtRefresh(void);

/* Components */

newtComponent newtButton(int left, int top, char * text);
newtComponent newtCheckbox(int left, int top, char * text, char defValue,
			   char * seq, char * result);
newtComponent newtRadiobutton(int left, int top, char * text, int isDefault,
			      newtComponent prevButton);
newtComponent newtRadioGetCurrent(newtComponent setMember);
newtComponent newtListitem(int left, int top, char * text, int isDefault,
			      newtComponent prevItem, void * data);
void newtListitemSet(newtComponent co, char * text);
void * newtListitemGetData(newtComponent co);

newtComponent newtLabel(int left, int top, char * text);
void newtLabelSetText(newtComponent co, char * text);
newtComponent newtVerticalScrollbar(int left, int top, int height,
				    int normalColorset, int thumbColorset);
void newtScrollbarSet(newtComponent co, int where, int total);

#define NEWT_LISTBOX_RETURNEXIT (1 << 0)
newtComponent newtListbox(int left, int top, int height, int flags);
/* return the data passed to AddEntry */
void * newtListboxGetCurrent(newtComponent co);
void newtListboxSetCurrent(newtComponent co, int num);
void newtListboxAddEntry(newtComponent co, char * text, void * data);
void newtListboxSetEntry(newtComponent co, int num, char * text);

#define NEWT_TEXTBOX_WRAP	(1 << 0)
#define NEWT_TEXTBOX_SCROLL	(1 << 1)

newtComponent newtTextbox(int left, int top, int with, int height, int flags);
void newtTextboxSetText(newtComponent co, const char * text);

#define NEWT_FORM_NOF12		(1 << 0)

struct newtExitStruct {
    enum { NEWT_EXIT_HOTKEY, NEWT_EXIT_COMPONENT } reason;
    union {
	int key;
	newtComponent co;
    } u;
} ;

newtComponent newtForm(newtComponent vertBar, char * help, int flags);
newtComponent newtFormGetCurrent(newtComponent co);
void newtFormSetBackground(newtComponent co, int color);
void newtFormSetCurrent(newtComponent co, newtComponent subco);
void newtFormAddComponent(newtComponent form, newtComponent co);
void newtFormAddComponents(newtComponent form, ...);
void newtFormSetHeight(newtComponent co, int height);
newtComponent newtRunForm(newtComponent form);		/* obsolete */
void newtFormRun(newtComponent co, struct newtExitStruct * es);
void newtDrawForm(newtComponent form);
void newtFormAddHotKey(newtComponent co, int key);

#define NEWT_ENTRY_SCROLL	(1 << 0)
#define NEWT_ENTRY_HIDDEN	(1 << 1)
#define NEWT_ENTRY_RETURNEXIT	(1 << 2)

newtComponent newtEntry(int left, int top, char * initialValue, int width,
			char ** resultPtr, int flags);
void newtEntrySet(newtComponent co, char * value, int cursorAtEnd);
void newtComponentAddCallback(newtComponent co, newtCallback f, void * data);

/* this also destroys all of the components (including other forms) on the 
   form */
void newtFormDestroy(newtComponent form);	

/* Key codes */

#define NEWT_KEY_TAB			'\t'
#define NEWT_KEY_ENTER			'\r'
#define NEWT_KEY_RETURN			NEWT_KEY_ENTER

#define NEWT_KEY_EXTRA_BASE		0x8000
#define NEWT_KEY_UP			NEWT_KEY_EXTRA_BASE + 1
#define NEWT_KEY_DOWN			NEWT_KEY_EXTRA_BASE + 2
#define NEWT_KEY_LEFT			NEWT_KEY_EXTRA_BASE + 4
#define NEWT_KEY_RIGHT			NEWT_KEY_EXTRA_BASE + 5
#define NEWT_KEY_BKSPC			NEWT_KEY_EXTRA_BASE + 6
#define NEWT_KEY_DELETE			NEWT_KEY_EXTRA_BASE + 7
#define NEWT_KEY_HOME			NEWT_KEY_EXTRA_BASE + 8
#define NEWT_KEY_END			NEWT_KEY_EXTRA_BASE + 9
#define NEWT_KEY_UNTAB			NEWT_KEY_EXTRA_BASE + 10
#define NEWT_KEY_PGUP			NEWT_KEY_EXTRA_BASE + 11
#define NEWT_KEY_PGDN			NEWT_KEY_EXTRA_BASE + 12

#define NEWT_KEY_F1			NEWT_KEY_EXTRA_BASE + 101
#define NEWT_KEY_F2			NEWT_KEY_EXTRA_BASE + 102
#define NEWT_KEY_F3			NEWT_KEY_EXTRA_BASE + 103
#define NEWT_KEY_F4			NEWT_KEY_EXTRA_BASE + 104
#define NEWT_KEY_F5			NEWT_KEY_EXTRA_BASE + 105
#define NEWT_KEY_F6			NEWT_KEY_EXTRA_BASE + 106
#define NEWT_KEY_F7			NEWT_KEY_EXTRA_BASE + 107
#define NEWT_KEY_F8			NEWT_KEY_EXTRA_BASE + 108
#define NEWT_KEY_F9			NEWT_KEY_EXTRA_BASE + 109
#define NEWT_KEY_F10			NEWT_KEY_EXTRA_BASE + 110
#define NEWT_KEY_F11			NEWT_KEY_EXTRA_BASE + 111
#define NEWT_KEY_F12			NEWT_KEY_EXTRA_BASE + 112

#endif /* H_NEWT */
