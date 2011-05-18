#include <slang.h>
#include <stdlib.h>
#include <string.h>

#include "newt.h"
#include "newt_pr.h"

struct label {
    char * text;
    int length;
    int cs;
};

static void labelDraw(newtComponent co);
static void labelDestroy(newtComponent co);

static struct componentOps labelOps = {
    labelDraw,
    newtDefaultEventHandler,
    labelDestroy,
    newtDefaultPlaceHandler,
    newtDefaultMappedHandler,
} ;

newtComponent newtLabel(int left, int top, const char * text) {
    newtComponent co;
    struct label * la;

    co = malloc(sizeof(*co));
    la = malloc(sizeof(struct label));
    co->data = la;
    co->destroyCallback = NULL;

    co->ops = &labelOps;

    co->height = 1;
    co->width = wstrlen(text, -1);
    co->top = top;
    co->left = left;
    co->takesFocus = 0;
    co->isMapped = 0;

    la->length = strlen(text);
    la->text = strdup(text);
    la->cs = COLORSET_LABEL;

    return co;
}

void newtLabelSetText(newtComponent co, const char * text) {
    int newLength;
    struct label * la = co->data;

    co->width = wstrlen(text,-1);
    newLength = strlen(text);
    if (newLength <= la->length) {
	memset(la->text, ' ', la->length);
	memcpy(la->text, text, newLength);
    } else {
	free(la->text);
	la->text = strdup(text);
	la->length = newLength;
    }

    labelDraw(co);
}

void newtLabelSetColors(newtComponent co, int colorset) {
    struct label * la = co->data;

    la->cs = colorset;
    labelDraw(co);
}

static void labelDraw(newtComponent co) {
    struct label * la = co->data;

    if (!co->isMapped) return;

    SLsmg_set_color(la->cs);

    newtGotorc(co->top, co->left);
    SLsmg_write_string(la->text);
}

static void labelDestroy(newtComponent co) {
    struct label * la = co->data;

    free(la->text);
    free(la);
    free(co);
}
