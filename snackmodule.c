#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "Python.h"
#include "newt.h"

typedef struct snackWidget_s snackWidget;
typedef struct snackGrid_s snackGrid;
typedef struct snackForm_s snackForm;

static void emptyDestructor(PyObject * s);

static snackWidget * buttonWidget(PyObject * s, PyObject * args);
static PyObject * centeredWindow(PyObject * s, PyObject * args);
static snackWidget * checkboxWidget(PyObject * s, PyObject * args);
static PyObject * choiceWindow(PyObject * s, PyObject * args);
static snackWidget * entryWidget(PyObject * s, PyObject * args);
static snackForm * formCreate(PyObject * s, PyObject * args);
static snackGrid * gridCreate(PyObject * s, PyObject * args);
static PyObject * gridWrappedWindow(PyObject * s, PyObject * args);
static PyObject * finishScreen(PyObject * s, PyObject * args);
static PyObject * initScreen(PyObject * s, PyObject * args);
static snackWidget * labelWidget(PyObject * s, PyObject * args);
static PyObject * messageWindow(PyObject * s, PyObject * args);
static PyObject * openWindow(PyObject * s, PyObject * args);
static PyObject * popWindow(PyObject * s, PyObject * args);
static snackWidget * radioButtonWidget(PyObject * s, PyObject * args);
static PyObject * refreshScreen(PyObject * s, PyObject * args);
static PyObject * ternaryWindow(PyObject * s, PyObject * args);

static PyMethodDef snackModuleMethods[] = {
    { "button", (PyCFunction) buttonWidget, METH_VARARGS, NULL },
    { "checkbox", (PyCFunction) checkboxWidget, METH_VARARGS, NULL },
    { "choice", choiceWindow, METH_VARARGS, NULL },
    { "centeredwindow", centeredWindow, METH_VARARGS, NULL },
    { "entry", (PyCFunction) entryWidget, METH_VARARGS, NULL },
    { "finish", finishScreen, METH_VARARGS, NULL },
    { "form", (PyCFunction) formCreate, METH_VARARGS, NULL },
    { "grid", (PyCFunction) gridCreate, METH_VARARGS, NULL },
    { "gridwrappedwindow", gridWrappedWindow, METH_VARARGS, NULL },
    { "init", initScreen, METH_VARARGS, NULL },
    { "label", (PyCFunction) labelWidget, METH_VARARGS, NULL },
    { "message", messageWindow, METH_VARARGS, NULL },
    { "openwindow", openWindow, METH_VARARGS, NULL },
    { "popwindow", popWindow, METH_VARARGS, NULL },
    { "radiobutton", (PyCFunction) radioButtonWidget, METH_VARARGS, NULL },
    { "refresh", refreshScreen, METH_VARARGS, NULL },
    { "ternary", ternaryWindow, METH_VARARGS, NULL },
    { NULL }
} ;

struct snackGrid_s {
    PyObject_HEAD;
    newtGrid grid;
} ;

static PyObject * gridGetAttr(PyObject * s, char * name);
static PyObject * gridPlace(snackGrid * s, PyObject * args);
static PyObject * gridSetField(snackGrid * s, PyObject * args);

static PyMethodDef gridMethods[] = {
    { "place", (PyCFunction) gridPlace, METH_VARARGS, NULL },
    { "setfield", (PyCFunction) gridSetField, METH_VARARGS, NULL },
    { NULL }
};

static PyTypeObject snackGridType = {
        PyObject_HEAD_INIT(&PyType_Type)
        0,                              /* ob_size */
        "snackgrid",                    /* tp_name */
        sizeof(snackGrid),              /* tp_size */
        0,                              /* tp_itemsize */
        emptyDestructor,      			/* tp_dealloc */
        0,                              /* tp_print */
        gridGetAttr,    		/* tp_getattr */
        0,                              /* tp_setattr */
        0,                              /* tp_compare */
        0,                              /* tp_repr */
        0,                              /* tp_as_number */
        0,                              /* tp_as_sequence */
        0,                		/* tp_as_mapping */
};

struct snackForm_s {
    PyObject_HEAD;
    newtComponent fo;
} ;

static PyObject * formGetAttr(PyObject * s, char * name);
static PyObject * formAdd(snackForm * s, PyObject * args);
static PyObject * formRun(snackForm * s, PyObject * args);

static PyMethodDef formMethods[] = {
    { "add", (PyCFunction) formAdd, METH_VARARGS, NULL },
    { "run", (PyCFunction) formRun, METH_VARARGS, NULL },
    { NULL }
};

static PyTypeObject snackFormType = {
        PyObject_HEAD_INIT(&PyType_Type)
        0,                              /* ob_size */
        "snackform",                    /* tp_name */
        sizeof(snackForm),              /* tp_size */
        0,                              /* tp_itemsize */
        emptyDestructor,      		/* tp_dealloc */
        0,                              /* tp_print */
        formGetAttr,    		/* tp_getattr */
        0,                              /* tp_setattr */
        0,                              /* tp_compare */
        0,                              /* tp_repr */
        0,                              /* tp_as_number */
        0,                              /* tp_as_sequence */
        0,                		/* tp_as_mapping */
};

struct snackWidget_s {
    PyObject_HEAD;
    newtComponent co;
    char achar;
    void * apointer;
} ;

static PyObject * widgetGetAttr(PyObject * s, char * name);
static PyObject * widgetEntrySetValue(snackWidget * s, PyObject * args);

static PyMethodDef widgetMethods[] = {
    { "entrySetValue", (PyCFunction) widgetEntrySetValue, METH_VARARGS, NULL },
    { NULL }
};

static PyTypeObject snackWidgetType = {
        PyObject_HEAD_INIT(&PyType_Type)
        0,                              /* ob_size */
        "snackwidget",                  /* tp_name */
        sizeof(snackWidget),            /* tp_size */
        0,                              /* tp_itemsize */
        emptyDestructor,      		/* tp_dealloc */
        0,                              /* tp_print */
        widgetGetAttr,  		/* tp_getattr */
        0,                              /* tp_setattr */
        0,                              /* tp_compare */
        0,                              /* tp_repr */
        0,                              /* tp_as_number */
        0,                              /* tp_as_sequence */
        0,                		/* tp_as_mapping */
};

static PyObject * initScreen(PyObject * s, PyObject * args) {
    newtInit();
    newtCls();

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * finishScreen(PyObject * s, PyObject * args) {
    newtFinished();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * refreshScreen(PyObject * s, PyObject * args) {
    newtRefresh();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * centeredWindow(PyObject * s, PyObject * args) {
    int width, height;
    char * title;

    if (!PyArg_ParseTuple(args, "iis", &width, &height, &title))
	return NULL;

    newtCenteredWindow(width, height, title);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * gridWrappedWindow(PyObject * s, PyObject * args) {
    snackGrid * grid;
    char * title;

    if (!PyArg_ParseTuple(args, "O!s", &snackGridType, &grid, &title))
	return NULL;

    newtGridWrappedWindow(grid->grid, title);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * openWindow(PyObject * s, PyObject * args) {
    int left, top, width, height;
    char * title;

    if (!PyArg_ParseTuple(args, "iiiis", &left, &top, &width, &height, &title))
	return NULL;

    newtOpenWindow(left, top, width, height, title);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * popWindow(PyObject * s, PyObject * args) {
    newtPopWindow();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * messageWindow(PyObject * s, PyObject * args) {
    char * title, * text;
    char * okbutton = "Ok";

    if (!PyArg_ParseTuple(args, "ss|s", &title, &text, &okbutton)) 
	return NULL;

    newtWinMessage(title, okbutton, text);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * choiceWindow(PyObject * s, PyObject * args) {
    char * title, * text;
    char * okbutton = "Ok";
    char * cancelbutton = "Cancel";
    int rc;

    if (!PyArg_ParseTuple(args, "ss|ss", &title, &text, &okbutton, 
			  &cancelbutton)) 
	return NULL;

    rc = newtWinChoice(title, okbutton, cancelbutton, text);

    switch (rc) {
      case 0: return Py_BuildValue("i", 1);
      case 1: return Py_BuildValue("i", 2);
    }

    return Py_BuildValue("i", 0);
}

static PyObject * ternaryWindow(PyObject * s, PyObject * args) {
    char * title, * text, * button1, * button2, * button3;
    int rc;

    if (!PyArg_ParseTuple(args, "sssss", &title, &text, &button1, &button2, 
			  &button3)) 
	return NULL;

    rc = newtWinTernary(title, button1, button2, button3, text);

    return Py_BuildValue("i", rc);
}

static snackWidget * buttonWidget(PyObject * s, PyObject * args) {
    snackWidget * widget;
    char * label;

    if (!PyArg_ParseTuple(args, "s", &label)) return NULL;

    widget = PyObject_NEW(snackWidget, &snackWidgetType);
    widget->co = newtButton(-1, -1, label);

    return widget;
}

static snackWidget * labelWidget(PyObject * s, PyObject * args) {
    char * label;
    snackWidget * widget;

    if (!PyArg_ParseTuple(args, "s", &label)) return NULL;

    widget = PyObject_NEW(snackWidget, &snackWidgetType);
    widget->co = newtLabel(-1, -1, label);

    return widget;
}

static snackWidget * radioButtonWidget(PyObject * s, PyObject * args) {
    snackWidget * widget, * group;
    char * text;
    int isOn;

    if (!PyArg_ParseTuple(args, "sOi", &text, &group, &isOn)) 
		return NULL;

    widget = PyObject_NEW(snackWidget, &snackWidgetType);

    if ((PyObject *) group == Py_None)
	widget->co = newtRadiobutton(-1, -1, text, isOn, NULL);
    else
	widget->co = newtRadiobutton(-1, -1, text, isOn, group->co);

    return widget;
}

static snackWidget * checkboxWidget(PyObject * s, PyObject * args) {
    snackWidget * widget;
    char * text;
    int isOn;

    if (!PyArg_ParseTuple(args, "si", &text, &isOn)) return NULL;

    widget = PyObject_NEW(snackWidget, &snackWidgetType);
    widget->co = newtCheckbox(-1, -1, text, isOn ? '*' : ' ', NULL, 
				&widget->achar);

    return widget;
}

static snackWidget * entryWidget(PyObject * s, PyObject * args) {
    snackWidget * widget;
    int width;
    char * initial;
    int isHidden, isScrolled;

    if (!PyArg_ParseTuple(args, "isii", &width, &initial,
			  &isHidden, &isScrolled)) return NULL;

    widget = PyObject_NEW(snackWidget, &snackWidgetType);
    widget->co = newtEntry(-1, -1, initial, width, (char **) &widget->apointer, 
			   (isHidden ? NEWT_FLAG_HIDDEN : 0) |
			   (!isScrolled ? NEWT_FLAG_NOSCROLL : 0));

    return widget;
}

static snackForm * formCreate(PyObject * s, PyObject * args) {
    snackForm * form;

    form = PyObject_NEW(snackForm, &snackFormType);
    form->fo = newtForm(NULL, NULL, 0);

    return form;
}

static snackGrid * gridCreate(PyObject * s, PyObject * args) {
    int rows, cols;
    snackGrid * grid;

    if (!PyArg_ParseTuple(args, "ii", &cols, &rows)) return NULL;

    grid = PyObject_NEW(snackGrid, &snackGridType);
    grid->grid = newtCreateGrid(cols, rows);

    return grid;
}

static PyObject * gridGetAttr(PyObject * s, char * name) {
    return Py_FindMethod(gridMethods, s, name);
}

static PyObject * gridPlace(snackGrid * grid, PyObject * args) {
    int x, y;

    if (!PyArg_ParseTuple(args, "ii", &x, &y)) return NULL;

    newtGridPlace(grid->grid, x, y);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * gridSetField(snackGrid * grid, PyObject * args) {
    snackWidget * w;
    int x, y;
    int pLeft = 0, pTop = 0, pRight = 0, pBottom = 0;

    if (!PyArg_ParseTuple(args, "iiO!|(iiii)", &x, &y, &snackWidgetType, 
				&w, &pLeft, &pTop, &pRight, &pBottom)) 
	return NULL;

    newtGridSetField(grid->grid, x, y, NEWT_GRID_COMPONENT,
		     w->co, pLeft, pTop, pRight, pBottom, 0, 0);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * formGetAttr(PyObject * s, char * name) {
    return Py_FindMethod(formMethods, s, name);
}

static PyObject * formAdd(snackForm * s, PyObject * args) {
    snackWidget * w;
    int size = PyTuple_Size(args), i;
    
    if (!size) {
	/* this is a hack, I should give an error directly */
	if (!PyArg_ParseTuple(args, "O!", &snackWidgetType, &w)) 
	    return NULL;
    }

    for (i = 0; i < size; i++) {
	w = (snackWidget *) PyTuple_GET_ITEM(args, i);
	newtFormAddComponent(s->fo, w->co);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * formRun(snackForm * s, PyObject * args) {
    newtRunForm(s->fo);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * widgetGetAttr(PyObject * s, char * name) {
    snackWidget * w = (snackWidget *) s;

    if (!strcmp(name, "radiobuttonkey")) {
	return Py_BuildValue("i", w->co);
    } else if (!strcmp(name, "entryValue")) {
	return Py_BuildValue("s", w->apointer);
    } else if (!strcmp(name, "checkboxValue")) {
	return Py_BuildValue("i", w->achar == ' ' ? 0 : 1);
    } else if (!strcmp(name, "radioValue")) {
	return Py_BuildValue("i", newtRadioGetCurrent(w->co));
    }

    return Py_FindMethod(widgetMethods, s, name);
}

static PyObject * widgetEntrySetValue(snackWidget * s, PyObject * args) {
    char * val;

    if (!PyArg_ParseTuple(args, "s", &val))
	return NULL;

    newtEntrySet(s->co, val, 1);

    Py_INCREF(Py_None);
    return Py_None;
}

static void emptyDestructor(PyObject * s) {
}

void init_snack(void) {
    Py_InitModule("_snack", snackModuleMethods);
}
