import _snack

class Widget:
    pass

class Button(Widget):

    def __init__(self, text):
	self.w = _snack.button(text)

class Checkbox(Widget):

    def value(self):
	return self.w.checkboxValue

    def selected(self):
	return self.w.checkboxValue != 0

    def __init__(self, text, isOn = 0):
	self.w = _snack.checkbox(text, isOn)

class SingleRadioButton(Widget):

    def selected(self):
	return self.w.key == self.w.radioValue;
    
    def __init__(self, text, group, isOn = 0):
	if group:
	    self.w = _snack.radiobutton(text, group.w, isOn)
	else:
	    self.w = _snack.radiobutton(text, None, isOn)

class Listbox(Widget):

    def append(self, text):
	return self.w.listboxAddItem(text)

    def current(self):
	return self.w.listboxGetCurrent()

    def __init__(self, height, scroll = 0, returnExit = 0, width = 0):
	self.w = _snack.listbox(height, scroll, returnExit)
	if (width):
	    self.w.listboxSetWidth(width)

class Textbox(Widget):

    def __init__(self, width, height, text, scroll = 0):
	self.w = _snack.textbox(width, height, text, scroll)

class Label(Widget):

    def __init__(self, text):
	self.w = _snack.label(text)

class Entry(Widget):

    def value(self):
	return self.w.entryValue

    def set(self, text):
	return self.w.entrySetValue(text)

    def __init__(self, width, text = "", hidden = 0, scroll = 1):
	self.w = _snack.entry(width, text, hidden, scroll)

class Form:

    def addHotKey(self, keyname):
	self.f.addhotkey(hotkeys[keyname])

    def add(self, widget):
	self.trans[widget.w.key] = widget
	return self.f.add(widget.w)

    def run(self):
	(what, which) = self.f.run()
	if (what == _snack.FORM_EXIT_WIDGET):
	    return self.trans[which]

	return hotkeys[which]

    def __init__(self):
	self.trans = {}
	self.f = _snack.form()

class Grid:

    def place(self, x, y):
	return self.g.place(x, y)

    def setField(self, what, col, row, padding = (0, 0, 0, 0),
		 anchorLeft = 0, anchorTop = 0, anchorRight = 0,
		 anchorBottom = 0, growx = 0, growy = 0):
	anchorFlags = 0
	if (anchorLeft):
	    anchorFlags = _snack.ANCHOR_LEFT
	elif (anchorRight):
	    anchorFlags = _snack.ANCHOR_RIGHT

	if (anchorTop):
	    anchorFlags = anchorFlags | _snack.ANCHOR_TOP
	elif (anchorBottom):
	    anchorFlags = anchorFlags | _snack.ANCHOR_BOTTOM

	gridFlags = 0
	if (growx):
	    gridFlags = _snack.GRID_GROWX
	if (growy):
	    gridFlags = gridFlags | _snack.GRID_GROWY

	if (what.__dict__.has_key('g')):
	    return self.g.setfield(col, row, what.g, padding, anchorFlags,
				   gridFlags)
	else:
	    return self.g.setfield(col, row, what.w, padding, anchorFlags)

    def __init__(self, *args):
	self.g = apply(_snack.grid, args)

class SnackScreen:

    def __init__(self):
	_snack.init()

    def finish(self):
	return _snack.finish()

    def openWindow(self, left, top, width, height, title):
	return _snack.openwindow(left, top, width, height, title)

    def centeredWindow(self, width, height, title):
	return _snack.centeredwindow(width, height, title)

    def gridWrappedWindow(self, grid, title):
	return _snack.gridwrappedwindow(grid.g, title)

    def popWindow(self):
	return _snack.popwindow()

    def refresh(self):
	return _snack.refresh()

# returns a tuple of the wrapped text, the actual width, and the actual height
def reflow(text, width, flexDown = 5, flexUp = 5):
    return _snack.reflow(text, width, flexDown, flexUp)


# combo widgets

def RadioGroup(Widget):

    def __init__(self):
	self.group = None
	self.buttonlist = []

    def add(self, title, value, default = None):
	if not self.group and default == None:
	    # If the first element is not explicitly set to
	    # not be the default, make it be the default
	    default = 1
	b = SingleRadioButton(title, self.group, default)
	if not self.group: self.group = b
	buttonlist.append((b, value))
	return b

    def getSelection(self):
	for (b, value) in self.buttonlist:
	    if b.selected: return value
	

# pack a ButtonBar with growx = 1

def ButtonBar(Grid):

    def __init__(self, screen, buttonlist):
	self.list = []
	self.item = 0
	Grid.__init__(self, len(buttonlist), 1)
	for (title, value) in buttonlist:
	    b = Button(title)
	    self.list.append(b, value)
	    self.setField(b, self.item, 0, (0, 1, 0, 1))
	    self.item = self.item + 1

# FIXME: need to make it possible to find what button was pressed...


def GridForm(Grid):

    def __init__(self, screen, title, *args):
	self.screen = screen
	self.title = title
	self.form = Form()
	args = list(args)
	args[:0] = self
	apply(Grid.__init__, args)

    def add(self, widget, col, row, padding = (0, 0, 0, 0),
            anchorLeft = 0, anchorTop = 0, anchorRight = 0,
            anchorBottom = 0, growx = 0, growy = 0):
	self.setField(widget, col, row, padding, anchorLeft,
		      anchorTop, anchorRight, anchorBottom,
		      growx, growy);
	self.form.add(widget)

    def run(self):
	self.screen.gridWrappedWindow(self.grid, title)
	self.form.run()
	self.screen.popWindow()

hotkeys = { "F1" : _snack.KEY_F1, "F2" : _snack.KEY_F2, "F3" : _snack.KEY_F3, 
            "F4" : _snack.KEY_F4, "F5" : _snack.KEY_F5, "F6" : _snack.KEY_F6, 
            "F7" : _snack.KEY_F7, "F8" : _snack.KEY_F8, "F9" : _snack.KEY_F9, 
            "F10" : _snack.KEY_F10, "F11" : _snack.KEY_F11, 
            "F12" : _snack.KEY_F12 }

for n in hotkeys.keys():
    hotkeys[hotkeys[n]] = n
