#ifndef H_NEWT
#define H_NEWT

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

newtComponent newtForm(newtComponent vertBar);
newtComponent newtFormGetCurrent(newtComponent co);
void newtFormSetCurrent(newtComponent co, newtComponent subco);
void newtFormAddComponent(newtComponent form, newtComponent co);
void newtFormAddComponents(newtComponent form, ...);
void newtFormSetHeight(newtComponent co, int height);
newtComponent newtRunForm(newtComponent form);
void newtDrawForm(newtComponent form);

#define NEWT_ENTRY_SCROLL	(1 << 0)
#define NEWT_ENTRY_HIDDEN	(1 << 1)
#define NEWT_ENTRY_RETURNEXIT	(1 << 2)

newtComponent newtEntry(int left, int top, char * initialValue, int width,
			char ** resultPtr, int flags);
void newtEntrySet(newtComponent co, char * value, int cursorAtEnd);
void newtEntryAddCallback(newtComponent co, newtCallback f, void * data);

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

#endif /* H_NEWT */
