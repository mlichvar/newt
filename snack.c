
#include "Python.h"
#include "config.h"

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#include "structmember.h"
#include "nls.h"
#include "newt.h"
#include "newt_pr.h"

#if PY_MAJOR_VERSION >= 3
  #define PyInt_FromLong PyLong_FromLong
  #define PyInt_AsLong PyLong_AsLong
  #define PyString_FromString PyUnicode_FromString
  #define MOD_ERROR_VAL NULL
  #define MOD_SUCCESS_VAL(val) val
  #define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
#else
  #define MOD_ERROR_VAL
  #define MOD_SUCCESS_VAL(val)
  #define MOD_INIT(name) void init##name(void)
#endif

typedef struct snackWidget_s snackWidget;
typedef struct snackGrid_s snackGrid;
typedef struct snackForm_s snackForm;

struct callbackStruct {
    PyObject * cb, * data;
};

/* Integer to pointer, 64-bit-sane */
#define I2P(x) ((void *)(long)(x))

static struct callbackStruct suspend;
static struct callbackStruct helpCallback;

static void emptyDestructor(PyObject * s);

static snackWidget * buttonWidget(PyObject * s, PyObject * args);
static snackWidget * compactbuttonWidget(PyObject * s, PyObject * args);
static PyObject * centeredWindow(PyObject * s, PyObject * args);
static snackWidget * checkboxWidget(PyObject * s, PyObject * args);
static PyObject * choiceWindow(PyObject * s, PyObject * args);
static snackWidget * entryWidget(PyObject * s, PyObject * args);
static PyObject * drawRootText(PyObject * s, PyObject * args);
static PyObject * doResume(PyObject * s, PyObject * args);
static PyObject * doSuspend(PyObject * s, PyObject * args);
static PyObject * doSuspend(PyObject * s, PyObject * args);
static snackForm * formCreate(PyObject * s, PyObject * args);
static snackGrid * gridCreate(PyObject * s, PyObject * args);
static PyObject * gridWrappedWindow(PyObject * s, PyObject * args);
static PyObject * finishScreen(PyObject * s, PyObject * args);
static PyObject * initScreen(PyObject * s, PyObject * args);
static snackWidget * labelWidget(PyObject * s, PyObject * args);
static snackWidget * listboxWidget(PyObject * s, PyObject * args);
static PyObject * messageWindow(PyObject * s, PyObject * args);
static PyObject * openWindow(PyObject * s, PyObject * args);
static PyObject * popHelpLine(PyObject * s, PyObject * args);
static PyObject * popWindow(PyObject * s, PyObject * args);
static PyObject * popWindowNoRefresh(PyObject * s, PyObject * args);
static PyObject * pushHelpLine(PyObject * s, PyObject * args);
static snackWidget * radioButtonWidget(PyObject * s, PyObject * args);
static PyObject * refreshScreen(PyObject * s, PyObject * args);
static PyObject * setColor(PyObject * s, PyObject * args);
static PyObject * scaleWidget(PyObject * s, PyObject * args);
static PyObject * scaleSet(snackWidget * s, PyObject * args);
static PyObject * screenSize(PyObject * s, PyObject * args);
static PyObject * setSuspendCallback(PyObject * s, PyObject * args);
static PyObject * setHelpCallback(PyObject * s, PyObject * args);
static PyObject * reflowText(PyObject * s, PyObject * args);
static snackWidget * textWidget(PyObject * s, PyObject * args);
static PyObject * ternaryWindow(PyObject * s, PyObject * args);
static snackWidget * checkboxTreeWidget(PyObject * s, PyObject * args, PyObject * kwargs);
static PyObject * pywstrlen(PyObject * s, PyObject * args);
static PyObject * widget_get_checkboxValue(PyObject *self, void *closure);
static PyObject * widget_get_radioValue(PyObject *self, void *closure);

static PyMethodDef snackModuleMethods[] = {
    { "button", (PyCFunction) buttonWidget, METH_VARARGS, NULL },
    { "compactbutton", (PyCFunction) compactbuttonWidget, METH_VARARGS, NULL },
    { "checkbox", (PyCFunction) checkboxWidget, METH_VARARGS, NULL },
    { "choice", choiceWindow, METH_VARARGS, NULL },
    { "centeredwindow", centeredWindow, METH_VARARGS, NULL },
    { "drawroottext", drawRootText, METH_VARARGS, NULL },
    { "entry", (PyCFunction) entryWidget, METH_VARARGS, NULL },
    { "finish", finishScreen, METH_VARARGS, NULL },
    { "form", (PyCFunction) formCreate, METH_VARARGS, NULL },
    { "grid", (PyCFunction) gridCreate, METH_VARARGS, NULL },
    { "gridwrappedwindow", gridWrappedWindow, METH_VARARGS, NULL },
    { "helpcallback", setHelpCallback, METH_VARARGS, NULL },
    { "init", initScreen, METH_VARARGS, NULL },
    { "label", (PyCFunction) labelWidget, METH_VARARGS, NULL },
    { "listbox", (PyCFunction) listboxWidget, METH_VARARGS, NULL },
    { "message", messageWindow, METH_VARARGS, NULL },
    { "openwindow", openWindow, METH_VARARGS, NULL },
    { "pophelpline", popHelpLine, METH_VARARGS, NULL },
    { "popwindow", popWindow, METH_VARARGS, NULL },
    { "popwindownorefresh", popWindowNoRefresh, METH_VARARGS, NULL },
    { "pushhelpline", pushHelpLine, METH_VARARGS, NULL },
    { "radiobutton", (PyCFunction) radioButtonWidget, METH_VARARGS, NULL },
    { "reflow", (PyCFunction) reflowText, METH_VARARGS, NULL },
    { "refresh", refreshScreen, METH_VARARGS, NULL },
    { "setcolor", setColor, METH_VARARGS, NULL },
    { "resume", doResume, METH_VARARGS, NULL },
    { "scale", scaleWidget, METH_VARARGS, NULL },
    { "size", screenSize, METH_VARARGS, NULL },
    { "suspend", doSuspend, METH_VARARGS, NULL },
    { "suspendcallback", setSuspendCallback, METH_VARARGS, NULL },
    { "ternary", ternaryWindow, METH_VARARGS, NULL },
    { "textbox", (PyCFunction) textWidget, METH_VARARGS, NULL },
    { "checkboxtree", (PyCFunction) checkboxTreeWidget, METH_VARARGS | METH_KEYWORDS, NULL },
    { "wstrlen", (PyCFunction) pywstrlen, METH_VARARGS | METH_KEYWORDS, NULL },
    { NULL }
} ;

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
        "_snack",            /* m_name */
        NULL,                /* m_doc */
        -1,                  /* m_size */
        snackModuleMethods,  /* m_methods */
        NULL,                /* m_reload */
        NULL,                /* m_traverse */
        NULL,                /* m_clear */
        NULL,                /* m_free */
    };
#endif

static struct PyGetSetDef widget_getset[] = {
        { "checkboxValue", widget_get_checkboxValue, 0, NULL, NULL },
        { "radioValue", widget_get_radioValue, 0, NULL, NULL },
        { NULL }
};

struct snackGrid_s {
    PyObject_HEAD
    newtGrid grid;
} ;

static PyObject * gridPlace(snackGrid * s, PyObject * args);
static PyObject * gridSetField(snackGrid * s, PyObject * args);

static PyMethodDef gridMethods[] = {
    { "place", (PyCFunction) gridPlace, METH_VARARGS, NULL },
    { "setfield", (PyCFunction) gridSetField, METH_VARARGS, NULL },
    { NULL }
};

static PyTypeObject snackGridType = {
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "snackgrid",                    /* tp_name */
        sizeof(snackGrid),              /* tp_size */
        0,                              /* tp_itemsize */
        emptyDestructor,      			/* tp_dealloc */
        0,                              /* tp_print */
        0,                   		/* tp_getattr */
        0,                              /* tp_setattr */
        0,                              /* tp_compare */
        0,                              /* tp_repr */
        0,                              /* tp_as_number */
        0,                              /* tp_as_sequence */
        0,                		/* tp_as_mapping */
	0,                              /* tp_hash */
	0,                              /* tp_call */
	0,                              /* tp_str */
	PyObject_GenericGetAttr,        /* tp_getattro */
	0,                              /* tp_setattro */
	0,                              /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,             /* tp_flags */
	0,                              /* tp_doc */
	0,                              /* tp_traverse */
	0,                              /* tp_clear */
	0,                              /* tp_richcompare */
	0,                              /* tp_weaklistoffset */
	0,                              /* tp_iter */
	0,                              /* tp_iternext */
	gridMethods                     /* tp_methods */
};

struct snackForm_s {
    PyObject_HEAD
    newtComponent fo;
} ;

static PyObject * formAdd(snackForm * s, PyObject * args);
static PyObject * formDraw(snackForm * s, PyObject * args);
static PyObject * formRun(snackForm * s, PyObject * args);
static PyObject * formHotKey(snackForm * s, PyObject * args);
static PyObject * formSetCurrent(snackForm * form, PyObject * args);
static PyObject * formSetTimer(snackForm * form, PyObject * args);
static PyObject * formWatchFD(snackForm * form, PyObject * args);

static PyMethodDef formMethods[] = {
    { "add", (PyCFunction) formAdd, METH_VARARGS, NULL },
    { "draw", (PyCFunction) formDraw, METH_VARARGS, NULL },
    { "run", (PyCFunction) formRun, METH_VARARGS, NULL },
    { "addhotkey", (PyCFunction) formHotKey, METH_VARARGS, NULL },
    { "setcurrent", (PyCFunction) formSetCurrent, METH_VARARGS, NULL },
    { "settimer", (PyCFunction) formSetTimer, METH_VARARGS, NULL },
    { "watchfd", (PyCFunction) formWatchFD, METH_VARARGS, NULL },
    { NULL }
};

static PyTypeObject snackFormType = {
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "snackform",                    /* tp_name */
        sizeof(snackForm),              /* tp_size */
        0,                              /* tp_itemsize */
        emptyDestructor,      		/* tp_dealloc */
        0,                              /* tp_print */
        0,                     		/* tp_getattr */
        0,                              /* tp_setattr */
        0,                              /* tp_compare */
        0,                              /* tp_repr */
        0,                              /* tp_as_number */
        0,                              /* tp_as_sequence */
        0,                		/* tp_as_mapping */
	0,                              /* tp_hash */
	0,                              /* tp_call */
	0,                              /* tp_str */
	PyObject_GenericGetAttr,        /* tp_getattro */
	0,                              /* tp_setattro */
	0,                              /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,             /* tp_flags */
	0,                              /* tp_doc */
	0,                              /* tp_traverse */
	0,                              /* tp_clear */
	0,                              /* tp_richcompare */
	0,                              /* tp_weaklistoffset */
	0,                              /* tp_iter */
	0,                              /* tp_iternext */
	formMethods                     /* tp_methods */
};

struct snackWidget_s {
    PyObject_HEAD
    newtComponent co;
    char achar;
    void * apointer;
    int anint;
    struct callbackStruct scs;
} ;

static PyObject * widgetAddCallback(snackWidget * s, PyObject * args);
static void widgetDestructor(PyObject * s);
static PyObject * widgetEntrySetValue(snackWidget * s, PyObject * args);
static PyObject * widgetLabelText(snackWidget * s, PyObject * args);
static PyObject * widgetLabelSetColors(snackWidget * s, PyObject * args);
static PyObject * widgetListboxSetW(snackWidget * s, PyObject * args);
static PyObject * widgetListboxAdd(snackWidget * s, PyObject * args);
static PyObject * widgetListboxIns(snackWidget * s, PyObject * args);
static PyObject * widgetListboxDel(snackWidget * s, PyObject * args);
static PyObject * widgetListboxGet(snackWidget * s, PyObject * args);
static PyObject * widgetListboxGetSel(snackWidget * s, PyObject * args);
static PyObject * widgetListboxSet(snackWidget * s, PyObject * args);
static PyObject * widgetListboxClear(snackWidget * s, PyObject * args);
static PyObject * widgetTextboxText(snackWidget * s, PyObject * args);
static PyObject * widgetTextboxHeight(snackWidget * s, PyObject * args);
static PyObject * widgetCheckboxTreeAddItem(snackWidget * s, PyObject * args);
static PyObject * widgetCheckboxTreeGetSel(snackWidget * s, PyObject * args);
static PyObject * widgetCheckboxTreeGetCur(snackWidget * s, PyObject * args);
static PyObject * widgetCheckboxTreeSetEntry(snackWidget * s, PyObject * args);
static PyObject * widgetCheckboxTreeSetWidth(snackWidget * s, PyObject * args);
static PyObject * widgetCheckboxTreeSetCurrent(snackWidget * s, PyObject * args);
static PyObject * widgetCheckboxTreeSetEntryValue(snackWidget * s, PyObject * args);
static PyObject * widgetCheckboxTreeGetEntryValue(snackWidget * s, PyObject * args);
static PyObject * widgetEntrySetFlags(snackWidget * s, PyObject * args);
static PyObject * widgetCheckboxSetFlags(snackWidget * s, PyObject * args);
static PyObject * widgetCheckboxSetValue(snackWidget * s, PyObject * args);

static PyMethodDef widgetMethods[] = {
    { "setCallback", (PyCFunction) widgetAddCallback, METH_VARARGS, NULL },
    { "labelSetColors", (PyCFunction) widgetLabelSetColors, METH_VARARGS, NULL },
    { "labelText", (PyCFunction) widgetLabelText, METH_VARARGS, NULL },
    { "textboxText", (PyCFunction) widgetTextboxText, METH_VARARGS, NULL },
    { "textboxHeight", (PyCFunction) widgetTextboxHeight, METH_VARARGS, NULL },
    { "entrySetValue", (PyCFunction) widgetEntrySetValue, METH_VARARGS, NULL },
    { "listboxAddItem", (PyCFunction) widgetListboxAdd, METH_VARARGS, NULL },
    { "listboxInsertItem", (PyCFunction) widgetListboxIns, METH_VARARGS, NULL },
    { "listboxGetCurrent", (PyCFunction) widgetListboxGet, METH_VARARGS, NULL },
    { "listboxGetSelection", (PyCFunction) widgetListboxGetSel, METH_VARARGS, NULL },
    { "listboxSetCurrent", (PyCFunction) widgetListboxSet, METH_VARARGS, NULL },
    { "listboxSetWidth", (PyCFunction) widgetListboxSetW, METH_VARARGS, NULL },
    { "listboxDeleteItem", (PyCFunction) widgetListboxDel, METH_VARARGS, NULL },
    { "listboxClear", (PyCFunction) widgetListboxClear, METH_VARARGS, NULL },
    { "scaleSet", (PyCFunction) scaleSet, METH_VARARGS, NULL },
    { "checkboxtreeAddItem", (PyCFunction) widgetCheckboxTreeAddItem,
      METH_VARARGS, NULL },
    { "checkboxtreeGetCurrent", (PyCFunction) widgetCheckboxTreeGetCur,
      METH_VARARGS, NULL },
    { "checkboxtreeGetEntryValue", (PyCFunction) widgetCheckboxTreeGetEntryValue,
      METH_VARARGS, NULL },
    { "checkboxtreeSetEntry", (PyCFunction) widgetCheckboxTreeSetEntry,
      METH_VARARGS, NULL },
    { "checkboxtreeSetWidth", (PyCFunction) widgetCheckboxTreeSetWidth, METH_VARARGS, NULL },
    { "checkboxtreeSetCurrent", (PyCFunction) widgetCheckboxTreeSetCurrent,
      METH_VARARGS, NULL },
    { "checkboxtreeSetEntryValue", (PyCFunction) widgetCheckboxTreeSetEntryValue,
      METH_VARARGS, NULL },
    { "checkboxtreeGetSelection", (PyCFunction) widgetCheckboxTreeGetSel,
      METH_VARARGS, NULL },  
    { "entrySetFlags", (PyCFunction) widgetEntrySetFlags, METH_VARARGS, NULL },
    { "checkboxSetFlags", (PyCFunction) widgetCheckboxSetFlags, METH_VARARGS, NULL },
    { "checkboxSetValue", (PyCFunction) widgetCheckboxSetValue, METH_VARARGS, NULL },
    { NULL }
};

static PyMemberDef widget_members[] = {
        { "key",
#if SIZEOF_VOID_P == SIZEOF_LONG
		T_LONG,
#elif SIZEOF_VOID_P == SIZEOF_LONG_LONG
		T_LONGLONG,
#else
#error unsupported pointer size
#endif
		offsetof(snackWidget, co), 0, NULL },
        { "entryValue", T_STRING, offsetof(snackWidget, apointer), 0, NULL },
	{ NULL }
};

static PyTypeObject snackWidgetType = {
        PyVarObject_HEAD_INIT(&PyType_Type, 0)
        "snackwidget",                  /* tp_name */
        sizeof(snackWidget),            /* tp_size */
        0,                              /* tp_itemsize */
        widgetDestructor,      		/* tp_dealloc */
        0,                              /* tp_print */
        0,                   		/* tp_getattr */
        0,                              /* tp_setattr */
        0,                              /* tp_compare */
        0,                              /* tp_repr */
        0,                              /* tp_as_number */
        0,                              /* tp_as_sequence */
        0,                		/* tp_as_mapping */
	0,                              /* tp_hash */
	0,                              /* tp_call */
	0,                              /* tp_str */
	PyObject_GenericGetAttr,        /* tp_getattro */
	0,                              /* tp_setattro */
	0,                              /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,             /* tp_flags */
	0,                              /* tp_doc */
	0,                              /* tp_traverse */
	0,                              /* tp_clear */
	0,                              /* tp_richcompare */
	0,                              /* tp_weaklistoffset */
	0,                              /* tp_iter */
	0,                              /* tp_iternext */
	widgetMethods,                  /* tp_methods */
	widget_members,                 /* tp_members */
	widget_getset                   /* tp_getset */
};

static snackWidget * snackWidgetNew (void) {
    snackWidget * widget;
     
    widget = PyObject_NEW(snackWidget, &snackWidgetType);
    if (!widget)
	return NULL;

    widget->scs.cb = NULL;
    widget->scs.data = NULL;

    return widget;
}

static PyObject * initScreen(PyObject * s, PyObject * args) {
    suspend.cb = NULL;
    suspend.data = NULL;
    
    newtInit();
    newtCls();

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * finishScreen(PyObject * s, PyObject * args) {
    Py_XDECREF (suspend.cb);
    Py_XDECREF (suspend.data);
    
    newtFinished();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * refreshScreen(PyObject * s, PyObject * args) {
    newtRefresh();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * setColor(PyObject * s, PyObject * args) {
    char * fg, * bg;
    int colorset;
    if (!PyArg_ParseTuple(args, "iss", &colorset, &fg, &bg))
	return NULL;

    newtSetColor(colorset, fg, bg);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * scaleWidget(PyObject * s, PyObject * args) {
    snackWidget * widget;
    int width, fullAmount;

    if (!PyArg_ParseTuple(args, "ii", &width, &fullAmount)) return NULL;

    widget = snackWidgetNew ();
    if (!widget)
	return NULL;
    widget->co = newtScale(-1, -1, width, fullAmount);

    return (PyObject *) widget;
}

static PyObject * scaleSet(snackWidget * s, PyObject * args) {
    int amount;

    if (!PyArg_ParseTuple(args, "i", &amount)) return NULL;

    newtScaleSet(s->co, amount);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * screenSize(PyObject * s, PyObject * args) {
    int width, height;

    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    newtGetScreenSize(&width, &height);

    return Py_BuildValue("(ii)", width, height);
}

static void helpCallbackMarshall(newtComponent co, void * data) {
    PyObject * args, * result;

    PyGILState_STATE _state = PyGILState_Ensure();

    args = Py_BuildValue("(O)", data);
    result = PyObject_CallObject(helpCallback.cb, args);
    Py_DECREF (args);
    Py_XDECREF(result);

    PyGILState_Release(_state);

    return;
}

static void suspendCallbackMarshall(void * data) {
    struct callbackStruct * scs = data;
    PyObject * args, * result;

    PyGILState_STATE _state = PyGILState_Ensure();

    if (scs->data) {
	args = Py_BuildValue("(O)", scs->data);
	result = PyObject_CallObject(scs->cb, args);
	Py_DECREF (args);
    } else
	result = PyObject_CallObject(scs->cb, NULL);
    
    if (!result) {
	PyErr_Print();
	PyErr_Clear();
    }

    Py_XDECREF(result);

    PyGILState_Release(_state);

    return;
}

static void callbackMarshall(newtComponent co, void * data) {
    struct callbackStruct * scs = data;
    PyObject * args, * result;

    PyGILState_STATE _state = PyGILState_Ensure();

    if (scs->data) {
	args = Py_BuildValue("(O)", scs->data);
	result = PyObject_CallObject(scs->cb, args);
	Py_DECREF (args);
    } else
	result = PyObject_CallObject(scs->cb, NULL);

    if (!result) {
	PyErr_Print();
	PyErr_Clear();
    }
    
    Py_XDECREF(result);

    PyGILState_Release(_state);

    return;
}

static PyObject * setSuspendCallback(PyObject * s, PyObject * args) {
    if (!PyArg_ParseTuple(args, "O|O", &suspend.cb, &suspend.data))
	return NULL;

    Py_INCREF (suspend.cb);
    Py_XINCREF (suspend.data);    
    
    newtSetSuspendCallback(suspendCallbackMarshall, &suspend);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * setHelpCallback(PyObject * s, PyObject * args) {
    if (!PyArg_ParseTuple(args, "O", &helpCallback.cb))
	return NULL;

    /*if (helpCallback.cb) {
	Py_DECREF (helpCallback.cb);
    }*/

    Py_INCREF (helpCallback.cb);

    newtSetHelpCallback(helpCallbackMarshall);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * drawRootText(PyObject * s, PyObject * args) {
    int left, top;
    char * text;

    if (!PyArg_ParseTuple(args, "iis", &left, &top, &text))
	return NULL;

    newtDrawRootText(left, top, text);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * doSuspend(PyObject * s, PyObject * args) {
    newtSuspend();

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * doResume(PyObject * s, PyObject * args) {
    newtResume();

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * popHelpLine(PyObject * s, PyObject * args) {
    newtPopHelpLine();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * pushHelpLine(PyObject * s, PyObject * args) {
    char * text;

    if (!PyArg_ParseTuple(args, "s", &text))
	return NULL;

    if (!strcmp(text, "*default*"))
	newtPushHelpLine(NULL);
    else
	newtPushHelpLine(text);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * reflowText(PyObject * s, PyObject * args) {
    char * text, * new;
    int width, minus = 5, plus = 5;
    int realWidth, realHeight;
    PyObject * tuple;

    if (!PyArg_ParseTuple(args, "si|ii", &text, &width, &minus, &plus))
	return NULL;

    new = newtReflowText(text, width, minus, plus, &realWidth, &realHeight);

    tuple = Py_BuildValue("(sii)", new, realWidth, realHeight);
    free(new);

    return tuple;
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
    int x = -1, y = -1;

    if (!PyArg_ParseTuple(args, "O!s|ii", &snackGridType, &grid, &title, 
			  &x, &y))
	return NULL;

    if (y == -1)
	newtGridWrappedWindow(grid->grid, title);
    else
	newtGridWrappedWindowAt(grid->grid, title, x, y);

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

static PyObject * popWindowNoRefresh(PyObject * s, PyObject * args) {
    newtPopWindowNoRefresh();
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * messageWindow(PyObject * s, PyObject * args) {
    char * title, * text;
    char * okbutton = "Ok";

    if (!PyArg_ParseTuple(args, "ss|s", &title, &text, &okbutton)) 
	return NULL;

    Py_BEGIN_ALLOW_THREADS
    newtWinMessage(title, okbutton, text);
    Py_END_ALLOW_THREADS

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

    Py_BEGIN_ALLOW_THREADS
    rc = newtWinChoice(title, okbutton, cancelbutton, text);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", rc);
}

static PyObject * ternaryWindow(PyObject * s, PyObject * args) {
    char * title, * text, * button1, * button2, * button3;
    int rc;

    if (!PyArg_ParseTuple(args, "sssss", &title, &text, &button1, &button2, 
			  &button3)) 
	return NULL;

    Py_BEGIN_ALLOW_THREADS
    rc = newtWinTernary(title, button1, button2, button3, text);
    Py_END_ALLOW_THREADS

    return Py_BuildValue("i", rc);
}

static snackWidget * buttonWidget(PyObject * s, PyObject * args) {
    snackWidget * widget;
    char * label;

    if (!PyArg_ParseTuple(args, "s", &label)) return NULL;

    widget = snackWidgetNew ();
    if (!widget)
	return NULL;
    widget->co = newtButton(-1, -1, label);

    return widget;
}

static snackWidget * compactbuttonWidget(PyObject * s, PyObject * args) {
    snackWidget * widget;
    char * label;

    if (!PyArg_ParseTuple(args, "s", &label)) return NULL;

    widget = snackWidgetNew ();
    if (!widget)
	return NULL;
    widget->co = newtCompactButton(-1, -1, label);

    return widget;
}

static snackWidget * labelWidget(PyObject * s, PyObject * args) {
    char * label;
    snackWidget * widget;

    if (!PyArg_ParseTuple(args, "s", &label)) return NULL;

    widget = snackWidgetNew ();
    if (!widget)
	return NULL;
    widget->co = newtLabel(-1, -1, label);

    return widget;
}

static PyObject * widgetLabelText(snackWidget * s, PyObject * args) {
    char * label;

    if (!PyArg_ParseTuple(args, "s", &label)) return NULL;

    newtLabelSetText(s->co, label);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * widgetLabelSetColors(snackWidget * s, PyObject * args) {
    int colorset;

    if (!PyArg_ParseTuple(args, "i", &colorset)) return NULL;

    newtLabelSetColors(s->co, colorset);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * widgetTextboxText(snackWidget * s, PyObject * args) {
    char * text;

    if (!PyArg_ParseTuple(args, "s", &text)) return NULL;

    newtTextboxSetText(s->co, text);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * widgetTextboxHeight(snackWidget * s, PyObject * args) {
    int height;

    if (!PyArg_ParseTuple(args, "i", &height)) return NULL;

    newtTextboxSetHeight(s->co, height);

    Py_INCREF(Py_None);
    return Py_None;
}

static snackWidget * listboxWidget(PyObject * s, PyObject * args) {
    snackWidget * widget;
    int height;
    int doScroll = 0, returnExit = 0, showCursor = 0, multiple = 0, border = 0;

    if (!PyArg_ParseTuple(args, "i|iiiii", &height, &doScroll, &returnExit, 
					   &showCursor, &multiple, &border))
	return NULL;

    widget = snackWidgetNew ();
    if (!widget)
	return NULL;
    widget->co = newtListbox(-1, -1, height,
			     (doScroll ? NEWT_FLAG_SCROLL : 0) |
			     (returnExit ? NEWT_FLAG_RETURNEXIT : 0) |
			     (showCursor ? NEWT_FLAG_SHOWCURSOR : 0) |
			     (multiple ? NEWT_FLAG_MULTIPLE : 0) |
			     (border ? NEWT_FLAG_BORDER : 0)
			     );
    widget->anint = 1;
    
    return widget;
}

static snackWidget * textWidget(PyObject * s, PyObject * args) {
    char * text;
    int width, height;
    int scrollBar = 0;
    int wrap = 0;
    snackWidget * widget;
    
    if (!PyArg_ParseTuple(args, "iis|ii", &width, &height, &text, &scrollBar, &wrap))
	return NULL;

    widget = snackWidgetNew ();
    if (!widget)
	return NULL;
    widget->co = newtTextbox(-1, -1, width, height,
				(scrollBar ? NEWT_FLAG_SCROLL : 0) |
 			        (wrap ? NEWT_FLAG_WRAP : 0));
    newtTextboxSetText(widget->co, text);
    
    return widget;
}

static snackWidget * radioButtonWidget(PyObject * s, PyObject * args) {
    snackWidget * widget, * group;
    char * text;
    int isOn;

    if (!PyArg_ParseTuple(args, "sOi", &text, &group, &isOn)) 
		return NULL;

    widget = snackWidgetNew ();
    if (!widget)
	return NULL;

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

    widget = snackWidgetNew ();
    if (!widget)
	return NULL;
    widget->co = newtCheckbox(-1, -1, text, isOn ? '*' : ' ', NULL, 
				&widget->achar);

    return widget;
}

static PyObject * widgetCheckboxSetFlags(snackWidget * s, PyObject * args) {
    int flag, sense;

    if (!PyArg_ParseTuple(args, "ii", &flag, &sense)) return NULL;

    newtCheckboxSetFlags(s->co, flag, sense);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * widgetCheckboxSetValue(snackWidget * s, PyObject * args) {
    char *value;

    if (!PyArg_ParseTuple(args, "s", &value)) return NULL;

    newtCheckboxSetValue(s->co, *value);
    
    Py_INCREF(Py_None);
    return Py_None;
}

static snackWidget * entryWidget(PyObject * s, PyObject * args) {
    snackWidget * widget;
    int width;
    char * initial;
    int isHidden, isScrolled, returnExit, isPassword;

    if (!PyArg_ParseTuple(args, "isiiii", &width, &initial,
			  &isHidden, &isPassword, &isScrolled, &returnExit)) return NULL;

    widget = snackWidgetNew ();
    if (!widget)
	return NULL;
    widget->co = newtEntry(-1, -1, initial, width,
                           (const char **) &widget->apointer, 
			   (isHidden ? NEWT_FLAG_HIDDEN : 0) |
			   (isPassword ? NEWT_FLAG_PASSWORD : 0) |
			   (returnExit ? NEWT_FLAG_RETURNEXIT : 0) |
			   (isScrolled ? NEWT_FLAG_SCROLL : 0));

    return widget;
}

static snackForm * formCreate(PyObject * s, PyObject * args) {
    snackForm * form;
    PyObject * help = Py_None;

    if (!PyArg_ParseTuple(args, "|O", &help)) return NULL;

    if (help == Py_None)
	help = NULL;

    form = PyObject_NEW(snackForm, &snackFormType);
    form->fo = newtForm(NULL, help, 0);

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

static PyObject * gridPlace(snackGrid * grid, PyObject * args) {
    int x, y;

    if (!PyArg_ParseTuple(args, "ii", &x, &y)) return NULL;

    newtGridPlace(grid->grid, x, y);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * gridSetField(snackGrid * grid, PyObject * args) {
    snackWidget * w;
    snackGrid * g;
    int x, y;
    int pLeft = 0, pTop = 0, pRight = 0, pBottom = 0;
    int anchorFlags = 0, growFlags = 0;

    if (!PyArg_ParseTuple(args, "iiO|(iiii)ii", &x, &y, 
				&w, &pLeft, &pTop, &pRight, &pBottom,
				&anchorFlags, &growFlags)) 
	return NULL;

    if (Py_TYPE(w) == &snackWidgetType) {
	newtGridSetField(grid->grid, x, y, NEWT_GRID_COMPONENT,
			 w->co, pLeft, pTop, pRight, pBottom, anchorFlags, 
			 growFlags);
    } else {
	g = (snackGrid *) w;
	newtGridSetField(grid->grid, x, y, NEWT_GRID_SUBGRID,
			 g->grid, pLeft, pTop, pRight, pBottom, anchorFlags, 
			 growFlags);
    }

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * formDraw(snackForm * s, PyObject * args) {
    newtDrawForm(s->fo);

    Py_INCREF(Py_None);
    return Py_None;
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
    struct newtExitStruct result;

    Py_BEGIN_ALLOW_THREADS
    newtFormRun(s->fo, &result);
    Py_END_ALLOW_THREADS

    if (result.reason == NEWT_EXIT_HOTKEY)
	return Py_BuildValue("(si)", "hotkey", result.u.key);
    else if (result.reason == NEWT_EXIT_TIMER)
	return Py_BuildValue("(si)", "timer", 0);
    else if (result.reason == NEWT_EXIT_FDREADY)
	return Py_BuildValue("(si)", "fdready", result.u.watch);
    else if (result.reason == NEWT_EXIT_COMPONENT)
#if SIZEOF_VOID_P <= SIZEOF_LONG
	return Py_BuildValue("(sl)", "widget", (long)result.u.co);
#else
	return Py_BuildValue("(sL)", "widget", (long long)result.u.co);
#endif
    else
	return Py_BuildValue("(si)", "error", 0);
}

static PyObject * formHotKey(snackForm * s, PyObject * args) {
    int key;

    if (!PyArg_ParseTuple(args, "i", &key))
	return NULL;

    newtFormAddHotKey(s->fo, key);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * formSetTimer(snackForm * form, PyObject * args) {
    int millisecs;

    if (!PyArg_ParseTuple(args, "i", &millisecs))
	return NULL;

    newtFormSetTimer(form->fo, millisecs);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * formWatchFD(snackForm * form, PyObject * args) {
    int fd, fdflags;

    if (!PyArg_ParseTuple(args, "ii", &fd, &fdflags))
	return NULL;

    newtFormWatchFd(form->fo, fd, fdflags);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * formSetCurrent(snackForm * form, PyObject * args) {
    snackWidget * w;

    if (!PyArg_ParseTuple(args, "O", &w))
	return NULL;

    newtFormSetCurrent(form->fo, w->co);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * widget_get_checkboxValue(PyObject *self, void *closure)
{
	snackWidget *w = (snackWidget *)self;

	return Py_BuildValue("i", w->achar == ' ' ? 0 : 1);
}

static PyObject * widget_get_radioValue(PyObject *self, void *closure)
{
	snackWidget *w = (snackWidget *)self;

#if SIZEOF_VOID_P <= SIZEOF_LONG
	return Py_BuildValue("l", (long)newtRadioGetCurrent(w->co));
#else
	return Py_BuildValue("L", (long long)newtRadioGetCurrent(w->co));
#endif
}

static void widgetDestructor(PyObject * o) {
    snackWidget * s = (snackWidget *) o;
    
    Py_XDECREF (s->scs.cb);
    Py_XDECREF (s->scs.data);
    
    PyObject_Free(o);
}

static PyObject * widgetAddCallback(snackWidget * s, PyObject * args) {
    s->scs.cb = NULL;
    s->scs.data = NULL;
    
    if (!PyArg_ParseTuple(args, "O|O", &s->scs.cb, &s->scs.data))
	return NULL;

    Py_INCREF (s->scs.cb);
    Py_XINCREF (s->scs.data);
    
    newtComponentAddCallback(s->co, callbackMarshall, &s->scs);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * widgetEntrySetValue(snackWidget * s, PyObject * args) {
    char * val;
    int cursorAtEnd = 1;

    if (!PyArg_ParseTuple(args, "s|i", &val, &cursorAtEnd))
	return NULL;

    newtEntrySet(s->co, val, cursorAtEnd);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * widgetEntrySetFlags(snackWidget * s, PyObject * args) {
    int flag, sense;

    if (!PyArg_ParseTuple(args, "ii", &flag, &sense)) return NULL;

    newtEntrySetFlags(s->co, flag, sense);
    
    Py_INCREF(Py_None);
    return Py_None;
}


static PyObject * widgetListboxAdd(snackWidget * s, PyObject * args) {
    char * text;
    
    if (!PyArg_ParseTuple(args, "s", &text))
	return NULL;

    newtListboxAddEntry(s->co, text, I2P(s->anint));

    return PyInt_FromLong(s->anint++);
}

static PyObject * widgetListboxIns(snackWidget * s, PyObject * args) {
    char * text;
    int key;
    
    if (!PyArg_ParseTuple(args, "si", &text, &key))
	return NULL;

    newtListboxInsertEntry(s->co, text, I2P(s->anint), I2P(key));

    return PyInt_FromLong(s->anint++);
}

static PyObject * widgetListboxDel(snackWidget * s, PyObject * args) {
    int key;
    
    if (!PyArg_ParseTuple(args, "i", &key))
	return NULL;

    newtListboxDeleteEntry(s->co, I2P(key));

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * widgetListboxGet(snackWidget * s, PyObject * args) {
    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    return PyInt_FromLong((long) newtListboxGetCurrent(s->co));
}

static PyObject * widgetListboxGetSel(snackWidget * s, PyObject * args) {
    void ** selection;
    int numselected;
    int i;
    PyObject * sel, * int_obj;

    if (!PyArg_ParseTuple(args, ""))
        return NULL;

    selection = (void **) newtListboxGetSelection(s->co, &numselected);

    sel = PyList_New(0);

    if (!selection) {
	return sel;
    }

    for (i = 0; i < numselected; i++) {
	int_obj = PyInt_FromLong((long) selection[i]);
	PyList_Append(sel, int_obj);
	Py_DECREF(int_obj);
    }
    free(selection);

    return sel;
}

static PyObject * widgetListboxSet(snackWidget * s, PyObject * args) {
    int index;
    
    if (!PyArg_ParseTuple(args, "i", &index))
	return NULL;

    newtListboxSetCurrentByKey(s->co, I2P(index));

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * widgetListboxSetW(snackWidget * s, PyObject * args) {
    int width;

    if (!PyArg_ParseTuple(args, "i", &width))
	return NULL;

    newtListboxSetWidth(s->co, width);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * widgetListboxClear(snackWidget * s, PyObject * args) {
  if (!PyArg_ParseTuple(args, ""))
    return NULL;

  newtListboxClear(s->co);

  Py_INCREF(Py_None);
  return Py_None;
}

static void emptyDestructor(PyObject * s) {
}

static snackWidget * checkboxTreeWidget(PyObject * s, PyObject * args, PyObject * kwargs) {
    int height;
    int scrollBar = 0;
    int hide_checkbox = 0;
    int unselectable = 0;
    int flags;
    snackWidget * widget;
    const char *kw[] = {"height", "scrollbar", "hide_checkbox", "unselectable", NULL};
    
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "i|iii", (char **) kw,
		&height, &scrollBar, &hide_checkbox, &unselectable))
	return NULL;

    flags = (scrollBar ? NEWT_FLAG_SCROLL : 0) |
	(hide_checkbox ? NEWT_CHECKBOXTREE_HIDE_BOX : 0) |    
	(unselectable ? NEWT_CHECKBOXTREE_UNSELECTABLE : 0);

    widget = snackWidgetNew ();
    if (!widget)
	return NULL;
    widget->co = newtCheckboxTree(-1, -1, height, flags);

    widget->anint = 1;

    return widget;
}

static PyObject * widgetCheckboxTreeAddItem(snackWidget * s, PyObject * args) {
    char * text;
    int selected = 0;
    PyObject * pathList, * o;
    int len;
    int * path;
    int i;

    if (!PyArg_ParseTuple(args, "sOi", &text, &pathList, &selected))
	return NULL;

    len = PyTuple_Size(pathList);
    path = alloca(sizeof(*path) * (len + 1));
    for (i = 0; i < len; i++) {
        o = PyTuple_GetItem(pathList, i);
	path[i] = PyInt_AsLong(o);
    }
    path[len] = NEWT_ARG_LAST;

    newtCheckboxTreeAddArray(s->co, text, I2P(s->anint),
    			     selected ? NEWT_FLAG_SELECTED : 0, path);

    return PyInt_FromLong(s->anint++);
}

static PyObject * widgetCheckboxTreeGetCur(snackWidget * s, PyObject * args) {
    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    return PyInt_FromLong((long)newtCheckboxTreeGetCurrent(s->co));
}

static PyObject * widgetCheckboxTreeSetEntry(snackWidget * s, PyObject * args) {
    int data;
    char *text;

    if (!PyArg_ParseTuple(args, "is", &data, &text)) return NULL;

    newtCheckboxTreeSetEntry(s->co, I2P(data), text);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * widgetCheckboxTreeSetWidth(snackWidget * s, PyObject * args) {
    int width;

    if (!PyArg_ParseTuple(args, "i", &width))
	return NULL;

    newtCheckboxTreeSetWidth(s->co, width);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * widgetCheckboxTreeSetCurrent(snackWidget * s, PyObject * args) {
    int data;

    if (!PyArg_ParseTuple(args, "i", &data)) return NULL;

    newtCheckboxTreeSetCurrent(s->co, I2P(data));

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * widgetCheckboxTreeSetEntryValue(snackWidget * s, PyObject * args) {
    int data;
    int isOn = 1;

    if (!PyArg_ParseTuple(args, "i|i", &data, &isOn)) return NULL;

    newtCheckboxTreeSetEntryValue(s->co, I2P(data),
				  isOn ? NEWT_CHECKBOXTREE_SELECTED :
					 NEWT_CHECKBOXTREE_UNSELECTED);

    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject * widgetCheckboxTreeGetEntryValue(snackWidget * s, PyObject * args) {
    int data;
    int isOn = 0;
    int isBranch = 0;
    char selection;

    if (!PyArg_ParseTuple(args, "i", &data)) return NULL;

    selection = newtCheckboxTreeGetEntryValue(s->co, I2P(data));

    if (selection == -1) {
	PyErr_SetString(PyExc_KeyError, "unknown entry");
	return NULL;
    }

    switch (selection) {
    case NEWT_CHECKBOXTREE_EXPANDED:
	isOn = 1;
    case NEWT_CHECKBOXTREE_COLLAPSED:
	isBranch = 1;
	break;
    case NEWT_CHECKBOXTREE_UNSELECTED:
	break;
    default:
	isOn = 1;
	break;
    }    
    return Py_BuildValue("(ii)", isBranch, isOn);
}

static PyObject * widgetCheckboxTreeGetSel(snackWidget * s,
					      PyObject * args) {
    void ** selection;
    int numselected;
    int i;
    PyObject * sel, * int_obj;

    if (!PyArg_ParseTuple(args, ""))
	return NULL;

    selection = (void **) newtCheckboxTreeGetSelection(s->co, &numselected);

    sel = PyList_New(0);
    
    if (!selection) {
	return sel;
    }

    for (i = 0; i < numselected; i++) {
	int_obj = PyInt_FromLong((long) selection[i]);
	PyList_Append(sel, int_obj);
	Py_DECREF(int_obj);
    }
    free(selection);

    return sel;
}

static PyObject * pywstrlen(PyObject * s, PyObject * args)
{
    char *str;
    int len = -1;
    
    if (!PyArg_ParseTuple(args, "s|i", &str, &len)) return NULL;

    return PyInt_FromLong(wstrlen(str, len));
}

static void setitemstring_decref(PyObject * dict,
				const char * s, PyObject * o)
{
    PyDict_SetItemString(dict, s, o);
    Py_DECREF(o);
}

MOD_INIT(_snack)
{
    PyObject * d, * m;

#if PY_MAJOR_VERSION >= 3
    m = PyModule_Create(&moduledef);
#else
    m = Py_InitModule("_snack", snackModuleMethods);
#endif

    if (!m)
	    return MOD_ERROR_VAL;

    d = PyModule_GetDict(m);

    setitemstring_decref(d, "ANCHOR_LEFT", PyInt_FromLong(NEWT_ANCHOR_LEFT));
    setitemstring_decref(d, "ANCHOR_TOP", PyInt_FromLong(NEWT_ANCHOR_TOP));
    setitemstring_decref(d, "ANCHOR_RIGHT", PyInt_FromLong(NEWT_ANCHOR_RIGHT));
    setitemstring_decref(d, "ANCHOR_BOTTOM",
			 PyInt_FromLong(NEWT_ANCHOR_BOTTOM));
    setitemstring_decref(d, "GRID_GROWX", PyInt_FromLong(NEWT_GRID_FLAG_GROWX));
    setitemstring_decref(d, "GRID_GROWY", PyInt_FromLong(NEWT_GRID_FLAG_GROWY));

    setitemstring_decref(d, "FD_READ", PyInt_FromLong(NEWT_FD_READ));
    setitemstring_decref(d, "FD_WRITE", PyInt_FromLong(NEWT_FD_WRITE));
    setitemstring_decref(d, "FD_EXCEPT", PyInt_FromLong(NEWT_FD_EXCEPT));

    setitemstring_decref(d, "FORM_EXIT_HOTKEY", PyString_FromString("hotkey"));
    setitemstring_decref(d, "FORM_EXIT_WIDGET", PyString_FromString("widget"));
    setitemstring_decref(d, "FORM_EXIT_TIMER", PyString_FromString("timer"));
    setitemstring_decref(d, "FORM_EXIT_FDREADY", PyString_FromString("fdready"));
    setitemstring_decref(d, "FORM_EXIT_ERROR", PyString_FromString("error"));

    setitemstring_decref(d, "KEY_TAB", PyInt_FromLong(NEWT_KEY_TAB));
    setitemstring_decref(d, "KEY_ENTER", PyInt_FromLong(NEWT_KEY_ENTER));
    setitemstring_decref(d, "KEY_SUSPEND", PyInt_FromLong(NEWT_KEY_SUSPEND));
    setitemstring_decref(d, "KEY_UP", PyInt_FromLong(NEWT_KEY_UP));
    setitemstring_decref(d, "KEY_DOWN", PyInt_FromLong(NEWT_KEY_DOWN));
    setitemstring_decref(d, "KEY_LEFT", PyInt_FromLong(NEWT_KEY_LEFT));
    setitemstring_decref(d, "KEY_RIGHT", PyInt_FromLong(NEWT_KEY_RIGHT));
    setitemstring_decref(d, "KEY_BACKSPACE", PyInt_FromLong(NEWT_KEY_BKSPC));
    setitemstring_decref(d, "KEY_DELETE", PyInt_FromLong(NEWT_KEY_DELETE));
    setitemstring_decref(d, "KEY_HOME", PyInt_FromLong(NEWT_KEY_HOME));
    setitemstring_decref(d, "KEY_END", PyInt_FromLong(NEWT_KEY_END));
    setitemstring_decref(d, "KEY_UNTAB", PyInt_FromLong(NEWT_KEY_UNTAB));
    setitemstring_decref(d, "KEY_PAGEUP", PyInt_FromLong(NEWT_KEY_PGUP));
    setitemstring_decref(d, "KEY_PAGEGDOWN", PyInt_FromLong(NEWT_KEY_PGDN));
    setitemstring_decref(d, "KEY_INSERT", PyInt_FromLong(NEWT_KEY_INSERT));
    setitemstring_decref(d, "KEY_F1", PyInt_FromLong(NEWT_KEY_F1));
    setitemstring_decref(d, "KEY_F2", PyInt_FromLong(NEWT_KEY_F2));
    setitemstring_decref(d, "KEY_F3", PyInt_FromLong(NEWT_KEY_F3));
    setitemstring_decref(d, "KEY_F4", PyInt_FromLong(NEWT_KEY_F4));
    setitemstring_decref(d, "KEY_F5", PyInt_FromLong(NEWT_KEY_F5));
    setitemstring_decref(d, "KEY_F6", PyInt_FromLong(NEWT_KEY_F6));
    setitemstring_decref(d, "KEY_F7", PyInt_FromLong(NEWT_KEY_F7));
    setitemstring_decref(d, "KEY_F8", PyInt_FromLong(NEWT_KEY_F8));
    setitemstring_decref(d, "KEY_F9", PyInt_FromLong(NEWT_KEY_F9));
    setitemstring_decref(d, "KEY_F10", PyInt_FromLong(NEWT_KEY_F10));
    setitemstring_decref(d, "KEY_F11", PyInt_FromLong(NEWT_KEY_F11));
    setitemstring_decref(d, "KEY_F12", PyInt_FromLong(NEWT_KEY_F12));
    setitemstring_decref(d, "KEY_ESC", PyInt_FromLong(NEWT_KEY_ESCAPE));
    setitemstring_decref(d, "KEY_RESIZE", PyInt_FromLong(NEWT_KEY_RESIZE));

    setitemstring_decref(d, "FLAG_DISABLED", PyInt_FromLong(NEWT_FLAG_DISABLED));
    setitemstring_decref(d, "FLAGS_SET", PyInt_FromLong(NEWT_FLAGS_SET));
    setitemstring_decref(d, "FLAGS_RESET", PyInt_FromLong(NEWT_FLAGS_RESET));
    setitemstring_decref(d, "FLAGS_TOGGLE", PyInt_FromLong(NEWT_FLAGS_TOGGLE));

    setitemstring_decref(d, "COLORSET_ROOT", PyInt_FromLong(NEWT_COLORSET_ROOT));
    setitemstring_decref(d, "COLORSET_BORDER", PyInt_FromLong(NEWT_COLORSET_BORDER));
    setitemstring_decref(d, "COLORSET_WINDOW", PyInt_FromLong(NEWT_COLORSET_WINDOW));
    setitemstring_decref(d, "COLORSET_SHADOW", PyInt_FromLong(NEWT_COLORSET_SHADOW));
    setitemstring_decref(d, "COLORSET_TITLE", PyInt_FromLong(NEWT_COLORSET_TITLE));
    setitemstring_decref(d, "COLORSET_BUTTON", PyInt_FromLong(NEWT_COLORSET_BUTTON));
    setitemstring_decref(d, "COLORSET_ACTBUTTON", PyInt_FromLong(NEWT_COLORSET_ACTBUTTON));
    setitemstring_decref(d, "COLORSET_CHECKBOX", PyInt_FromLong(NEWT_COLORSET_CHECKBOX));
    setitemstring_decref(d, "COLORSET_ACTCHECKBOX", PyInt_FromLong(NEWT_COLORSET_ACTCHECKBOX));
    setitemstring_decref(d, "COLORSET_ENTRY", PyInt_FromLong(NEWT_COLORSET_ENTRY));
    setitemstring_decref(d, "COLORSET_LABEL", PyInt_FromLong(NEWT_COLORSET_LABEL));
    setitemstring_decref(d, "COLORSET_LISTBOX", PyInt_FromLong(NEWT_COLORSET_LISTBOX));
    setitemstring_decref(d, "COLORSET_ACTLISTBOX", PyInt_FromLong(NEWT_COLORSET_ACTLISTBOX));
    setitemstring_decref(d, "COLORSET_TEXTBOX", PyInt_FromLong(NEWT_COLORSET_TEXTBOX));
    setitemstring_decref(d, "COLORSET_ACTTEXTBOX", PyInt_FromLong(NEWT_COLORSET_ACTTEXTBOX));
    setitemstring_decref(d, "COLORSET_HELPLINE", PyInt_FromLong(NEWT_COLORSET_HELPLINE));
    setitemstring_decref(d, "COLORSET_ROOTTEXT", PyInt_FromLong(NEWT_COLORSET_ROOTTEXT));
    setitemstring_decref(d, "COLORSET_EMPTYSCALE", PyInt_FromLong(NEWT_COLORSET_EMPTYSCALE));
    setitemstring_decref(d, "COLORSET_FULLSCALE", PyInt_FromLong(NEWT_COLORSET_FULLSCALE));
    setitemstring_decref(d, "COLORSET_DISENTRY", PyInt_FromLong(NEWT_COLORSET_DISENTRY));
    setitemstring_decref(d, "COLORSET_COMPACTBUTTON", PyInt_FromLong(NEWT_COLORSET_COMPACTBUTTON));
    setitemstring_decref(d, "COLORSET_ACTSELLISTBOX", PyInt_FromLong(NEWT_COLORSET_ACTSELLISTBOX));
    setitemstring_decref(d, "COLORSET_SELLISTBOX", PyInt_FromLong(NEWT_COLORSET_SELLISTBOX));

    return MOD_SUCCESS_VAL(m);
}
