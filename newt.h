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
};

extern struct newtColors newtDefaultColorPalette;

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

typedef struct newtComponent * newtComponent;

newtComponent newtButton(int left, int top, char * text);
newtComponent newtCheckbox(int left, int top, char * text, char defValue,
			   char * seq, char * result);
newtComponent newtRadiobutton(int left, int top, char * text, int isDefault,
			      newtComponent prevButton);
newtComponent newtListitem(int left, int top, char * text, int isDefault,
			      newtComponent prevItem);

newtComponent newtLabel(int left, int top, char * text);

newtComponent newtListbox(int left, int top, int height, int flags);
void newtListboxAddEntry(newtComponent co, char * text);

newtComponent newtForm(void);
void newtFormAddComponent(newtComponent form, newtComponent co);
void newtFormAddComponents(newtComponent form, ...);
void newtFormSetHeight(newtComponent co, int height);
newtComponent newtRunForm(newtComponent form);

#define NEWT_ENTRY_SCROLL	(1 << 0)
#define NEWT_ENTRY_HIDDEN	(1 << 1)

newtComponent newtEntry(int left, int top, char * initialValue, int width,
			char ** resultPtr, int flags);


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
