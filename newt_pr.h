#ifndef H_NEWT_PR
#define H_NEWT_PR

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

    newtCallback callback;
    void * callbackData;

    void * data;
} ;

enum eventResultTypes { ER_IGNORED, ER_SWALLOWED, ER_EXITFORM, ER_SETFOCUS,
			ER_NEXTCOMP };
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
