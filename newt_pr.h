#ifndef H_NEWT_PR
#define H_NEWT_PR

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

void newtGotorc(int row, int col);
void newtDrawBox(int left, int top, int width, int height, int shadow);
void newtClearBox(int left, int top, int width, int height);

int newtGetKey(void);

struct newtComponent {
    /* common data */
    int height, width; 
    int top, left;
    int takesFocus;

    struct componentOps * ops;

    void * data;
} ;

enum eventResultTypes { ER_IGNORED, ER_SWALLOWED, ER_EXITFORM, ER_SETFOCUS };
struct eventResult {
    enum eventResultTypes result;
    union {
	newtComponent focus;
    } u;
}; 

enum eventTypes { EV_FOCUS, EV_UNFOCUS, EV_KEYPRESS };
enum eventSequence { EV_EARLY, EV_NORMAL, EV_LATE };

struct event {
    enum eventTypes event;
    enum eventSequence when;
    union {
	int key;
    } u;
} ;

struct componentOps {
    void (* draw)(struct newtComponent * c);
    struct eventResult (* event)(struct newtComponent * c, struct event ev);
    void (* destroy)(struct newtComponent * c);
} ;

struct eventResult newtDefaultEventHandler(struct newtComponent * c, 
					   struct event ev);

#endif /* H_NEWT_PR */
