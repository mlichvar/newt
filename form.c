#include "config.h"

#include <sys/types.h>

#include <slang.h>
#include <stdarg.h>
#include <stdlib.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#include <sys/time.h>

#ifdef USE_GPM
#include <ctype.h>
#include <sys/time.h>      /* timeval */
#include <sys/socket.h>    /* socket() */
#include <sys/un.h>        /* struct sockaddr_un */
#include <sys/fcntl.h>     /* O_RDONLY */
#include <sys/stat.h>      /* stat() */
#include <termios.h>       /* winsize */
#include <unistd.h>
#include <sys/kd.h>        /* KDGETMODE */
#include <signal.h>
#include <stdio.h>
#endif

#include "newt.h"
#include "newt_pr.h"

#ifdef USE_GPM
/*....................................... The connection data structure */

typedef struct Gpm_Connect {
  unsigned short eventMask, defaultMask;
  unsigned short minMod, maxMod;
  int pid;
  int vc;
}              Gpm_Connect;

/*....................................... Stack struct */
typedef struct Gpm_Stst {
  Gpm_Connect info;
  struct Gpm_Stst *next;
} Gpm_Stst;

enum Gpm_Etype {
  GPM_MOVE=1,
  GPM_DRAG=2,   /* exactly one of the bare ones is active at a time */
  GPM_DOWN=4,
  GPM_UP=  8,

#define GPM_BARE_EVENTS(type) ((type)&(0x0f|GPM_ENTER|GPM_LEAVE))

  GPM_SINGLE=16,            /* at most one in three is set */
  GPM_DOUBLE=32,
  GPM_TRIPLE=64,            /* WARNING: I depend on the values */

  GPM_MFLAG=128,            /* motion during click? */
  GPM_HARD=256,             /* if set in the defaultMask, force an already
                   used event to pass over to another handler */

  GPM_ENTER=512,            /* enter event, user in Roi's */
  GPM_LEAVE=1024            /* leave event, used in Roi's */
};

/*....................................... The reported event */

enum Gpm_Margin {GPM_TOP=1, GPM_BOT=2, GPM_LFT=4, GPM_RGT=8};

typedef struct Gpm_Event {
  unsigned char buttons, modifiers;  /* try to be a multiple of 4 */
  unsigned short vc;
  short dx, dy, x, y;
  enum Gpm_Etype type;
  int clicks;
  enum Gpm_Margin margin;
}              Gpm_Event;

static int Gpm_Open(Gpm_Connect *conn, int flag);
static int Gpm_Close(void);

static int gpm_fd=-1;
static int gpm_flag=0;
static int gpm_tried=0;
Gpm_Stst *gpm_stack=NULL;
static char *gpm_sock_name=NULL;
static struct sigaction gpm_saved_suspend_hook;
static struct sigaction gpm_saved_winch_hook;

#define GPM_XTERM_ON
#define GPM_XTERM_OFF
#define GPM_NODE_DEV "/dev/gpmctl"
#define GPM_NODE_CTL GPM_NODE_DEV

static inline int putdata(int where,  Gpm_Connect *what)
{
  if (write(where,what,sizeof(Gpm_Connect))!=sizeof(Gpm_Connect))
    {
      return -1;
    }
  return 0;
}

static void gpm_winch_hook (int signum)
{
  if (SIG_IGN != gpm_saved_winch_hook.sa_handler &&
      SIG_DFL != gpm_saved_winch_hook.sa_handler) {
    gpm_saved_winch_hook.sa_handler(signum);
  } /*if*/
}

static void gpm_suspend_hook (int signum)
{
  Gpm_Connect gpm_connect;
  sigset_t old_sigset;
  sigset_t new_sigset;
  struct sigaction sa;
  int success;

  sigemptyset (&new_sigset);
  sigaddset (&new_sigset, SIGTSTP);
  sigprocmask (SIG_BLOCK, &new_sigset, &old_sigset);

  /* Open a completely transparent gpm connection */
  gpm_connect.eventMask = 0;
  gpm_connect.defaultMask = ~0;
  gpm_connect.minMod = ~0;
  gpm_connect.maxMod = 0;
  /* cannot do this under xterm, tough */
  success = (Gpm_Open (&gpm_connect, 0) >= 0);

  /* take the default action, whatever it is (probably a stop :) */
  sigprocmask (SIG_SETMASK, &old_sigset, 0);
  sigaction (SIGTSTP, &gpm_saved_suspend_hook, 0);
  kill (getpid (), SIGTSTP);

  /* in bardo here */

  /* Reincarnation. Prepare for another death early. */
  sigemptyset(&sa.sa_mask);
  sa.sa_handler = gpm_suspend_hook;
  sa.sa_flags = SA_NOMASK;
  sigaction (SIGTSTP, &sa, 0);

  /* Pop the gpm stack by closing the useless connection */
  /* but do it only when we know we opened one.. */
  if (success) {
    Gpm_Close ();
  } /*if*/
}

static int Gpm_Open(Gpm_Connect *conn, int flag)
{
  char tty[32];
  char *term;
  int i;
  struct sockaddr_un addr;
  Gpm_Stst *new;

  /*....................................... First of all, check xterm */

  if ((term=(char *)getenv("TERM")) && !strncmp(term,"xterm",5))
    {
      if (gpm_tried) return gpm_fd; /* no stack */
      gpm_fd=-2;
      GPM_XTERM_ON;
      gpm_flag=1;
      return gpm_fd;
    }
  /*....................................... No xterm, go on */


  /*
   * So I chose to use the current tty, instead of /dev/console, which
   * has permission problems. (I am fool, and my console is
   * readable/writeable by everybody.
   *
   * However, making this piece of code work has been a real hassle.
   */

  if (!gpm_flag && gpm_tried) return -1;
  gpm_tried=1; /* do or die */

  new=malloc(sizeof(Gpm_Stst));
  if (!new) return -1;

  new->next=gpm_stack;
  gpm_stack=new;

  conn->pid=getpid(); /* fill obvious values */

  if (new->next)
    conn->vc=new->next->info.vc; /* inherit */
  else
    {
      conn->vc=0;                 /* default handler */
      if (flag>0)
        {  /* forced vc number */
          conn->vc=flag;
          sprintf(tty,"/dev/tty%i",flag);
        }
      else if (flag==0)  /* use your current vc */
        {
          char *t = ttyname(0); /* stdin */
          if (!t) t = ttyname(1); /* stdout */
          if (!t) goto err;
          strcpy(tty,t);
          if (strncmp(tty,"/dev/tty",8) || !isdigit(tty[8]))
            goto err;
          conn->vc=atoi(tty+8);
        }
      else /* a default handler -- use console */
        sprintf(tty,"/dev/tty0");

    }

  new->info=*conn;

  /*....................................... Connect to the control socket */

  if (!(gpm_flag++))
    {

      if ( (gpm_fd=socket(AF_UNIX,SOCK_STREAM,0))<0 )
        {
          goto err;
        }

      bzero((char *)&addr,sizeof(addr));
      addr.sun_family=AF_UNIX;
      if (!(gpm_sock_name = tempnam (0, "gpm"))) {
        goto err;
      } /*if*/
      strncpy (addr.sun_path, gpm_sock_name, sizeof (addr.sun_path));
      if (bind (gpm_fd, (struct sockaddr*)&addr,
                sizeof (addr.sun_family) + strlen (addr.sun_path))==-1) {
        goto err;
      } /*if*/

      bzero((char *)&addr,sizeof(addr));
      addr.sun_family=AF_UNIX;
      strcpy(addr.sun_path, GPM_NODE_CTL);
      i=sizeof(addr.sun_family)+strlen(GPM_NODE_CTL);

      if ( connect(gpm_fd,(struct sockaddr *)(&addr),i)<0 )
        {
          struct stat stbuf;

          /*
           * Well, try to open a chr device called /dev/gpmctl. This should
           * be forward-compatible with a kernel server
           */
          close(gpm_fd); /* the socket */
          if ((gpm_fd=open(GPM_NODE_DEV,O_RDWR))==-1) {
            goto err;
          } /*if*/
          if (fstat(gpm_fd,&stbuf)==-1 || (stbuf.st_mode&S_IFMT)!=S_IFCHR)
            goto err;
        }
    }
  /*....................................... Put your data */

  if (putdata(gpm_fd,conn)!=-1)
    {
      /* itz Wed Dec 16 23:22:16 PST 1998 use sigaction, the old
         code caused a signal loop under XEmacs */
      struct sigaction sa;
      sigemptyset(&sa.sa_mask);

#if (defined(SIGWINCH))
      /* And the winch hook .. */
      sa.sa_handler = gpm_winch_hook;
      sa.sa_flags = 0;
      sigaction(SIGWINCH, &sa, &gpm_saved_winch_hook);
#endif

#if (defined(SIGTSTP))
      if (gpm_flag == 1) {
        /* Install suspend hook */
        sa.sa_handler = SIG_IGN;
        sigaction(SIGTSTP, &sa, &gpm_saved_suspend_hook);

        /* if signal was originally ignored, job control is not supported */
        if (gpm_saved_suspend_hook.sa_handler != SIG_IGN) {
          sa.sa_flags = SA_NOMASK;
          sa.sa_handler = gpm_suspend_hook;
          sigaction(SIGTSTP, &sa, 0);
        } /*if*/
      } /*if*/
#endif

    } /*if*/
  return gpm_fd;

  /*....................................... Error: free all memory */
 err:
  do
    {
      new=gpm_stack->next;
      free(gpm_stack);
      gpm_stack=new;
    }
  while(gpm_stack);
  if (gpm_fd>=0) close(gpm_fd);
  if (gpm_sock_name) {
    unlink(gpm_sock_name);
    free(gpm_sock_name);
    gpm_sock_name = NULL;
  } /*if*/
  gpm_flag=0;
  gpm_fd=-1;
  return -1;
}

/*-------------------------------------------------------------------*/
static int Gpm_Close(void)
{
  Gpm_Stst *next;

  gpm_tried=0; /* reset the error flag for next time */
  if (gpm_fd==-2) /* xterm */
    GPM_XTERM_OFF;
  else            /* linux */
    {
      if (!gpm_flag) return 0;
      next=gpm_stack->next;
      free(gpm_stack);
      gpm_stack=next;
      if (next)
        putdata(gpm_fd,&(next->info));

      if (--gpm_flag) return -1;
    }

  if (gpm_fd>=0) close(gpm_fd);
  gpm_fd=-1;
  if (gpm_sock_name) {
    unlink(gpm_sock_name);
    free(gpm_sock_name);
    gpm_sock_name = NULL;
  }
#ifdef SIGTSTP
  sigaction(SIGTSTP, &gpm_saved_suspend_hook, 0);
#endif
#ifdef SIGWINCH
  sigaction(SIGWINCH, &gpm_saved_winch_hook, 0);
#endif
  return 0;
}

/*-------------------------------------------------------------------*/
static int Gpm_GetEvent(Gpm_Event *event)
{
  int count;

  if (!gpm_flag) return 0;

  if ((count=read(gpm_fd,event,sizeof(Gpm_Event)))!=sizeof(Gpm_Event))
    {
      if (count==0)
        {
          Gpm_Close();
          return 0;
        }
      return -1;
    }
  return 1;
}
#endif

/****************************************************************************
    These forms handle vertical scrolling of components with a height of 1

    Horizontal scrolling won't work, and scrolling large widgets will fail
    miserably. It shouldn't be too hard to fix either of those if anyone
    cares to. I only use scrolling for listboxes and text boxes though so
    I didn't bother.
*****************************************************************************/

struct element {
    newtComponent co;
};

struct fdInfo {
    int fd;
    int flags;
};

struct form {
    int numCompsAlloced;
    struct element * elements;
    int numComps;
    int currComp;
    int fixedHeight;
    int flags;
    int vertOffset;
    newtComponent vertBar, exitComp;
    const char * help;
    int numRows;
    int * hotKeys;
    int numHotKeys;
    int background;
    int numFds;
    struct fdInfo * fds;
    int maxFd;
    int timer;    /* in milliseconds */
    struct timeval lastTimeout;
    void * helpTag;
    newtCallback helpCb;
};

static void gotoComponent(newtComponent co, int newComp);
static struct eventResult formEvent(newtComponent co, struct event ev);
static struct eventResult sendEvent(newtComponent comp, struct event ev);
static void formPlace(newtComponent co, int left, int top);

/* Global, ick */
static newtCallback helpCallback;

/* this isn't static as grid.c tests against it to find forms */
struct componentOps formOps = {
    newtDrawForm,
    formEvent,
    newtFormDestroy,
    formPlace,
    newtDefaultMappedHandler,
} ;

int needResize = 0;

static inline int componentFits(newtComponent co, int compNum) {
    struct form * form = co->data;
    struct element * el = form->elements + compNum;

    if (co->top > el->co->top)
	return 0;
    if (co->top + co->height < el->co->top + el->co->height)
	return 0;

    return 1;
}

newtComponent newtForm(newtComponent vertBar, void * help, int flags) {
    newtComponent co;
    struct form * form;

    co = malloc(sizeof(*co));
    form = malloc(sizeof(*form));
    co->data = form;
    co->width = 0;
    co->height = 0;
    co->top = -1;
    co->left = -1;
    co->isMapped = 0;

    co->takesFocus = 0;			/* we may have 0 components */
    co->ops = &formOps;
    co->callback = NULL;
    co->destroyCallback = NULL;

    form->help = help;
    form->flags = flags;
    form->numCompsAlloced = 5;
    form->numComps = 0;
    form->currComp = -1;
    form->vertOffset = 0;
    form->fixedHeight = 0;
    form->numRows = 0;
    form->numFds = 0;
    form->maxFd = 0;
    form->fds = NULL;
    form->elements = malloc(sizeof(*(form->elements)) * form->numCompsAlloced);

    form->background = COLORSET_WINDOW;
    form->hotKeys = malloc(sizeof(int));
    form->numHotKeys = 0;
    form->timer = 0;
    form->lastTimeout.tv_sec = form->lastTimeout.tv_usec = 0;
    if (!(form->flags & NEWT_FLAG_NOF12)) {
	newtFormAddHotKey(co, NEWT_KEY_F12);
    }

    if (vertBar)
	form->vertBar = vertBar;
    else
	form->vertBar = NULL;

    form->helpTag = help;
    form->helpCb = helpCallback;

    return co;
}

newtComponent newtFormGetCurrent(newtComponent co) {
    struct form * form = co->data;

    if (form->currComp == -1) return 0;
    return form->elements[form->currComp].co;
}

static void formScroll(newtComponent co, int delta) {
    struct form * form = co->data;
    struct element * el;
    int i, newVertOffset = form->vertOffset + delta;

    if (newVertOffset < 0)
	newVertOffset = 0;
    if (newVertOffset > form->numRows - co->height)
	newVertOffset = form->numRows - co->height;

    delta = newVertOffset - form->vertOffset;
    form->vertOffset = newVertOffset;

    for (i = 0, el = form->elements; i < form->numComps; i++, el++) {
	if (el->co == form->vertBar)
	    continue;
	el->co->ops->place(el->co, el->co->left, el->co->top - delta);
    }
}

int newtFormGetScrollPosition(newtComponent co) {
    struct form * form = co->data;

    return form->vertOffset;
}

void newtFormSetScrollPosition(newtComponent co, int position) {
    struct form * form = co->data;

    if (form->numRows == 0)
	newtFormSetSize(co);
    formScroll(co, position - form->vertOffset);
}

void newtFormSetCurrent(newtComponent co, newtComponent subco) {
    struct form * form = co->data;
    int i, new;

    for (i = 0; i < form->numComps; i++) {
	 if (form->elements[i].co == subco) break;
    }

    if (form->elements[i].co != subco) return;
    new = i;

    if (co->isMapped && !componentFits(co, new)) {
	gotoComponent(co, -1);
	formScroll(co, form->elements[new].co->top - co->top - 1);
    }

    gotoComponent(co, new);
}

void newtFormSetTimer(newtComponent co, int millisecs) {
    struct form * form = co->data;

    form->timer = millisecs;
    form->lastTimeout.tv_usec = 0;
    form->lastTimeout.tv_sec = 0;
}

void newtFormSetHeight(newtComponent co, int height) {
    struct form * form = co->data;

    form->fixedHeight = 1;
    co->height = height;
}

void newtFormSetWidth(newtComponent co, int width) {
    co->width = width;
}

void newtFormAddComponent(newtComponent co, newtComponent newco) {
    struct form * form = co->data;

    co->takesFocus = 1;

    if (form->numCompsAlloced == form->numComps) {
	form->numCompsAlloced += 5;
	form->elements = realloc(form->elements,
			    sizeof(*(form->elements)) * form->numCompsAlloced);
    }

    form->elements[form->numComps].co = newco;

    if (newco->takesFocus && form->currComp == -1)
	form->currComp = form->numComps;

    form->numComps++;
}

void newtFormAddComponents(newtComponent co, ...) {
    va_list ap;
    newtComponent subco;

    va_start(ap, co);

    while ((subco = va_arg(ap, newtComponent)))
	newtFormAddComponent(co, subco);

    va_end(ap);
}

static void formPlace(newtComponent co, int left, int top) {
    struct form * form = co->data;
    int vertDelta, horizDelta;
    struct element * el;
    int i;

    vertDelta = top - co->top;
    horizDelta = left - co->left;
    co->top = top;
    co->left = left;

    for (i = 0, el = form->elements; i < form->numComps; i++, el++) {
	el->co->ops->place(el->co, el->co->left + horizDelta,
		el->co->top + vertDelta);
    }
}

void newtDrawForm(newtComponent co) {
    struct form * form = co->data;
    struct element * el;
    int i;

    newtFormSetSize(co);

    SLsmg_set_color(form->background);
    newtClearBox(co->left, co->top, co->width, co->height);

    for (i = 0, el = form->elements; i < form->numComps; i++, el++) {
	/* only draw it if it'll fit on the screen vertically
	   (the scrollbar *always* fits somewhere) */
	if (el->co == form->vertBar || componentFits(co, i)) {
	    el->co->ops->mapped(el->co, 1);
	    el->co->ops->draw(el->co);
	} else {
	    el->co->ops->mapped(el->co, 0);
	}
    }

    if (form->vertBar)
	newtScrollbarSet(form->vertBar, form->vertOffset,
			 form->numRows - co->height);
}

static struct eventResult formEvent(newtComponent co, struct event ev) {
    struct form * form = co->data;
    newtComponent subco = form->elements[form->currComp].co;
    int new, wrap = 0;
    struct eventResult er;
    int dir = 0, page = 0;
    int i, num, found;
    struct element * el;

    er.result = ER_IGNORED;
    if (!form->numComps) return er;

    if (form->currComp == -1) return er;

    switch (ev.when) {
      case EV_EARLY:
	  if (ev.event == EV_KEYPRESS) {
	    if (ev.u.key == NEWT_KEY_TAB) {
		er.result = ER_SWALLOWED;
		dir = 1;
		wrap = 1;
	    } else if (ev.u.key == NEWT_KEY_UNTAB) {
		er.result = ER_SWALLOWED;
		dir = -1;
		wrap = 1;
	    }
	}

	if (form->numComps) {
	    i = form->currComp;
	    num = 0;
	    while (er.result == ER_IGNORED && num != form->numComps ) {
		er = form->elements[i].co->ops->event(form->elements[i].co, ev);

		num++;
		i++;
		if (i == form->numComps) i = 0;
	    }
	}

	break;

      case EV_NORMAL:
	  if (ev.event == EV_MOUSE) {
	      found = 0;
	      for (i = 0, el = form->elements; i < form->numComps; i++, el++) {
		  if ((el->co->top <= ev.u.mouse.y) &&
		      (el->co->top + el->co->height > ev.u.mouse.y) &&
		      (el->co->left <= ev.u.mouse.x) &&
		      (el->co->left + el->co->width > ev.u.mouse.x)) {
		      found = 1;
		      if (el->co->takesFocus) {
			  gotoComponent(co, i);
			  subco = form->elements[form->currComp].co;
		      }
		  }
		  /* If we did not find a co to send this event to, we
		     should just swallow the event here. */
	      }
	      if (!found) {
		  er.result = ER_SWALLOWED;

		  return er;
	      }
	  }
	er = subco->ops->event(subco, ev);
	switch (er.result) {
	  case ER_NEXTCOMP:
	    er.result = ER_SWALLOWED;
	    dir = 1;
	    break;

	  case ER_EXITFORM:
	    form->exitComp = subco;
	    break;

	  default:
	    break;
	}
	break;

      case EV_LATE:
	er = subco->ops->event(subco, ev);

	if (er.result == ER_IGNORED) {
	    switch (ev.u.key) {
	      case NEWT_KEY_UP:
	      case NEWT_KEY_LEFT:
	      case NEWT_KEY_BKSPC:
		er.result = ER_SWALLOWED;
		dir = -1;
		break;

	      case NEWT_KEY_DOWN:
	      case NEWT_KEY_RIGHT:
		er.result = ER_SWALLOWED;
		dir = 1;
		break;

	     case NEWT_KEY_PGUP:
		er.result = ER_SWALLOWED;
		dir = -1;
		page = 1;
		break;

	     case NEWT_KEY_PGDN:
		er.result = ER_SWALLOWED;
		dir = 1;
		page = 1;
		break;
	    }
	}
    }

    if (dir) {
	new = form->currComp;

	if (page) {
	    new += dir * co->height;
	    if (new < 0)
		new = 0;
	    else if (new >= form->numComps)
		new = (form->numComps - 1);

	    while (!form->elements[new].co->takesFocus &&
		    new - dir >= 0 && new - dir < form->numComps)
		new -= dir;
	} else {
	    do {
		new += dir;

		if (wrap) {
		    if (new < 0)
			new = form->numComps - 1;
		    else if (new >= form->numComps)
			new = 0;
		    if (new == form->currComp)
			/* back where we started */
			return er;
		} else if (new < 0 || new >= form->numComps)
		    return er;
	    } while (!form->elements[new].co->takesFocus);
	}

	/* make sure this component is visible */
	if (!componentFits(co, new)) {
	    int vertDelta;

	    gotoComponent(co, -1);

	    if (dir < 0) {
		/* make the new component the first one */
		vertDelta = form->elements[new].co->top - co->top;
	    } else {
		/* make the new component the last one */
		vertDelta = (form->elements[new].co->top +
					form->elements[new].co->height) -
				    (co->top + co->height);
	    }

	    formScroll(co, vertDelta);
	    newtDrawForm(co);
	}

	gotoComponent(co, new);
	er.result = ER_SWALLOWED;
    }

    return er;
}

/* Destroy a component.  Components which have been added to a form
 * are destroyed when the form is destroyed; this is just for the
 * (rare) case of components which for whatever reason weren't added
 * to a form.
 */
void newtComponentDestroy(newtComponent co) {
    /* If the user registered a destroy callback for this component,
     * now is a good time to call it.
     */
    if (co->destroyCallback)
        co->destroyCallback(co, co->destroyCallbackData);

    if (co->ops->destroy) {
        co->ops->destroy(co);
    } else {
        if (co->data) free(co->data);
	free(co);
    }
}

/* this also destroys all of the components on the form */
void newtFormDestroy(newtComponent co) {
    newtComponent subco;
    struct form * form = co->data;
    int i;

    /* first, destroy all of the components */
    for (i = 0; i < form->numComps; i++) {
	subco = form->elements[i].co;
	newtComponentDestroy(subco);
    }

    if (form->hotKeys) free(form->hotKeys);

    free(form->elements);
    free(form);
    free(co);
}

newtComponent newtRunForm(newtComponent co) {
    struct newtExitStruct es;

    newtFormRun(co, &es);
    if (es.reason == NEWT_EXIT_HOTKEY) {
	if (es.u.key == NEWT_KEY_F12) {
	    es.reason = NEWT_EXIT_COMPONENT;
	    es.u.co = co;
	} else {
	    return NULL;
	}
    } else if (es.reason == NEWT_EXIT_ERROR)
	return NULL;

    return es.u.co;
}

void newtFormAddHotKey(newtComponent co, int key) {
    struct form * form = co->data;

    form->numHotKeys++;
    form->hotKeys = realloc(form->hotKeys, sizeof(int) * form->numHotKeys);
    form->hotKeys[form->numHotKeys - 1] = key;
}

void newtFormSetSize(newtComponent co) {
    struct form * form = co->data;
    int delta, i, first;
    struct element * el;

    form->numRows = 0;

    co->width = 0;
    if (!form->fixedHeight) co->height = 0;

    co->top = -1;
    co->left = -1;
    first = 1;

    for (i = 0, el = form->elements; i < form->numComps; i++, el++) {
	if (el->co->ops == &formOps)
	    newtFormSetSize(el->co);
	else if (el->co == form->vertBar)
	    continue;

	if (first) {
	    co->top = el->co->top;
	    co->left = el->co->left;
	    first = 0;
	}

	if (co->left > el->co->left) {
	    delta = co->left - el->co->left;
	    co->left -= delta;
	    co->width += delta;
	}

	if (co->top > el->co->top) {
	    delta = co->top - el->co->top;
	    co->top -= delta;
	    form->numRows += delta;
	    if (!form->fixedHeight)
		co->height += delta;
	}

	if ((co->left + co->width) < (el->co->left + el->co->width))
	    co->width = (el->co->left + el->co->width) - co->left;

	if (!form->fixedHeight) {
	    if ((co->top + co->height) < (el->co->top + el->co->height))
		co->height = (el->co->top + el->co->height) - co->top;
	}

	if ((el->co->top + el->co->height - co->top) > form->numRows) {
	    form->numRows = el->co->top + el->co->height - co->top;
	}
    }

    co->top += form->vertOffset;
}

void newtFormRun(newtComponent co, struct newtExitStruct * es) {
    struct form * form = co->data;
    struct event ev;
    struct eventResult er;
    int key, i, max;
    int done = 0;
    fd_set readSet, writeSet, exceptSet;
    struct timeval nextTimeout, now, timeout;
#ifdef USE_GPM
    int x, y;
    Gpm_Connect conn;
    Gpm_Event event;

    /* Set up GPM interface */
    conn.eventMask   = ~GPM_MOVE;
    conn.defaultMask = GPM_MOVE;
    conn.minMod      = 0;
    conn.maxMod      = 0;

    Gpm_Open(&conn, 0);
#endif

    /* draw all of the components */
    newtDrawForm(co);

    if (form->currComp == -1) {
	if (form->numComps)
	    gotoComponent(co, 0);
    } else
	gotoComponent(co, form->currComp);

    while (!done) {
	newtRefresh();

	FD_ZERO(&readSet);
	FD_ZERO(&writeSet);
	FD_ZERO(&exceptSet);
	FD_SET(0, &readSet);
#ifdef USE_GPM
	if (gpm_fd > 0) {
	    FD_SET(gpm_fd, &readSet);
	}
	max = form->maxFd > gpm_fd ? form->maxFd : gpm_fd;
#else
	max = form->maxFd;
#endif

	for (i = 0; i < form->numFds; i++) {
	    if (form->fds[i].flags & NEWT_FD_READ)
		FD_SET(form->fds[i].fd, &readSet);
	    if (form->fds[i].flags & NEWT_FD_WRITE)
		FD_SET(form->fds[i].fd, &writeSet);
	    if (form->fds[i].flags & NEWT_FD_EXCEPT)
		FD_SET(form->fds[i].fd, &exceptSet);
	}

	if (form->timer) {
	    /* Calculate when we next need to return with a timeout. Do
	       this inside the loop in case a callback resets the timer. */
	    gettimeofday(&now, 0);

	    if ((!form->lastTimeout.tv_sec && !form->lastTimeout.tv_usec) ||
		    now.tv_sec < form->lastTimeout.tv_sec ||
		    (now.tv_sec == form->lastTimeout.tv_sec &&
		     now.tv_usec < form->lastTimeout.tv_usec))
		form->lastTimeout = now;

	    nextTimeout.tv_sec = form->lastTimeout.tv_sec + 
		    (form->timer / 1000);
	    nextTimeout.tv_usec = form->lastTimeout.tv_usec + 
				    (form->timer % 1000) * 1000;

	    if (now.tv_sec > nextTimeout.tv_sec) {
		timeout.tv_sec = timeout.tv_usec = 0;
	    } else if (now.tv_sec == nextTimeout.tv_sec) {
		timeout.tv_sec = 0;
		if (now.tv_usec > nextTimeout.tv_usec)
		    timeout.tv_usec = 0;
		else
		    timeout.tv_usec = nextTimeout.tv_usec - now.tv_usec;
	    } else if (now.tv_sec < nextTimeout.tv_sec) {
		timeout.tv_sec = nextTimeout.tv_sec - now.tv_sec;
		if (now.tv_usec > nextTimeout.tv_usec)
		    timeout.tv_sec--,
		    timeout.tv_usec = nextTimeout.tv_usec + 1000000 -
					now.tv_usec;
		else 
		    timeout.tv_usec = nextTimeout.tv_usec - now.tv_usec;
	    }
	} else {
	    timeout.tv_sec = timeout.tv_usec = 0;
	}

	if (needResize) {
		needResize = 0;
		newtResizeScreen(1);
	}

	i = select(max + 1, &readSet, &writeSet, &exceptSet, 
			form->timer ? &timeout : NULL);
	if (i < 0) continue;	/* ?? What should we do here? */

	if (i == 0) {
	    done = 1;
	    es->reason = NEWT_EXIT_TIMER;
	    gettimeofday(&form->lastTimeout, NULL);
	} else
#ifdef USE_GPM
	if (gpm_fd > 0 && FD_ISSET(gpm_fd, &readSet)) {
	    Gpm_GetEvent(&event);

	    if (event.type & GPM_DOWN) {
		/* Transform coordinates to current window */
		newtGetWindowPos(&x, &y);

		ev.event = EV_MOUSE;
		ev.u.mouse.type = MOUSE_BUTTON_DOWN;
		ev.u.mouse.x = event.x - x - 1;
		ev.u.mouse.y = event.y - y - 1;

		/* Send the form the event */
		er = sendEvent(co, ev);

		if (er.result == ER_EXITFORM) {
		    done = 1;
		    es->reason = NEWT_EXIT_COMPONENT;
		    es->u.co = form->exitComp;
		}

	    }
	} else
#endif
	{
	    if (FD_ISSET(0, &readSet)) {

		key = newtGetKey();

		for (i = 0; i < form->numHotKeys; i++) {
		    if (form->hotKeys[i] == key) {
			es->reason = NEWT_EXIT_HOTKEY;
			es->u.key = key;
			done = 1;
			break;
		    }
		}

		if (key == NEWT_KEY_F1 && form->helpTag && form->helpCb) {
		    if (form->currComp != -1) {
			ev.event = EV_UNFOCUS;
			sendEvent(form->elements[form->currComp].co, ev);
		    }
		    form->helpCb(co, form->helpTag);
		    if (form->currComp != -1) {
			ev.event = EV_FOCUS;
			sendEvent(form->elements[form->currComp].co, ev);
		    }
		}

		if (key == NEWT_KEY_ERROR) {
		    es->u.watch = -1;
		    es->reason = NEWT_EXIT_ERROR;
		    done = 1;
		}

		if (!done) {
		    ev.event = EV_KEYPRESS;
		    ev.u.key = key;

		    er = sendEvent(co, ev);

		    if (er.result == ER_EXITFORM) {
			done = 1;
			es->reason = NEWT_EXIT_COMPONENT;
			es->u.co = form->exitComp;
		    }
		}
	    } else {
		for (i = 0; i < form->numFds; i++) {
		    if (((form->fds[i].flags & NEWT_FD_READ)
			&& FD_ISSET(form->fds[i].fd, &readSet))
			|| ((form->fds[i].flags & NEWT_FD_WRITE)
			&& FD_ISSET(form->fds[i].fd, &writeSet))
			|| ((form->fds[i].flags & NEWT_FD_EXCEPT)
			&& FD_ISSET(form->fds[i].fd, &exceptSet))) break;
		}
		if(i < form->numFds)
		    es->u.watch = form->fds[i].fd;
		else
		    es->u.watch = -1;

		es->reason = NEWT_EXIT_FDREADY;
		done = 1;
	    }
	}
    }
    newtRefresh();
#ifdef USE_GPM
    Gpm_Close();
#endif
}

static struct eventResult sendEvent(newtComponent co, struct event ev) {
    struct eventResult er;

    ev.when = EV_EARLY;
    er = co->ops->event(co, ev);

    if (er.result == ER_IGNORED) {
	ev.when = EV_NORMAL;
	er = co->ops->event(co, ev);
    }

    if (er.result == ER_IGNORED) {
	ev.when = EV_LATE;
	er = co->ops->event(co, ev);
    }

    return er;
}

static void gotoComponent(newtComponent co, int newComp) {
    struct form * form = co->data;
    struct event ev;

    if (form->currComp != -1) {
	ev.event = EV_UNFOCUS;
	sendEvent(form->elements[form->currComp].co, ev);
    }

    form->currComp = newComp;

    if (form->currComp != -1) {
	ev.event = EV_FOCUS;
	ev.when = EV_NORMAL;
	sendEvent(form->elements[form->currComp].co, ev);
    }

    if (co->callback)
	co->callback(co, co->callbackData);
}

void newtComponentAddCallback(newtComponent co, newtCallback f, void * data) {
    co->callback = f;
    co->callbackData = data;
}

/* Add a callback which is called when the component is destroyed. */
void newtComponentAddDestroyCallback(newtComponent co,
				newtCallback f, void * data) {
    co->destroyCallback = f;
    co->destroyCallbackData = data;
}

void newtComponentTakesFocus(newtComponent co, int val) {
    co->takesFocus = val;
}

void newtFormSetBackground(newtComponent co, int color) {
    struct form * form = co->data;

    form->background = color;
}

void newtFormWatchFd(newtComponent co, int fd, int fdFlags) {
    struct form * form = co->data;
    int i;

    for (i = 0; i < form->numFds; i++)
      if (form->fds[i].fd == fd)
	break;

    if(i >= form->numFds)
      form->fds = realloc(form->fds, (++form->numFds) * sizeof(*form->fds));

    form->fds[i].fd = fd;
    form->fds[i].flags = fdFlags;
    if (form->maxFd < fd) form->maxFd = fd;
}

void newtSetHelpCallback(newtCallback cb) {
    helpCallback = cb;
}
