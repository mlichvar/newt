#include <slang/slang.h>
#include <stdlib.h>
#include <string.h>

#include "newt.h"
#include "newt_pr.h"

struct label {
    char * text;
};

static void labelDraw(newtComponent co);
static void labelDestroy(newtComponent co);

static struct componentOps labelOps = {
    labelDraw,
    newtDefaultEventHandler,
    labelDestroy,
} ;

newtComponent newtLabel(int left, int top, char * text) {
    newtComponent co;
    struct label * la;

    co = malloc(sizeof(*co));
    la = malloc(sizeof(struct label));
    co->data = la;

    co->ops = &labelOps;

    co->height = 1;
    co->width = strlen(text);
    co->top = top;
    co->left = left;
    co->takesFocus = 0;

    la->text = strdup(text);

    return co;
}

static void labelDraw(newtComponent co) {
    struct label * la = co->data;

    if (co->top == -1) return;

    SLsmg_set_color(COLORSET_LABEL);

    newtGotorc(co->top, co->left);
    SLsmg_write_string(la->text);
}

static void labelDestroy(newtComponent co) {
    struct label * la = co->data;

    free(la->text);
    free(la);
    free(co);
}
