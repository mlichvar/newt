# snack.py: maps C extension module _snack to proper python types in module
# snack.
# The first section is a very literal mapping.
# The second section contains convenience classes that amalgamate
# the literal classes and make them more object-oriented.

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

    def append(self, text, item):
	key = self.w.listboxAddItem(text)
	self.key2item[key] = item
	self.item2key[item] = key

    def insert(self, text, item, before):
	if (not before):
	    key = self.w.listboxInsertItem(text, 0)
	else:
	    key = self.w.listboxInsertItem(text, self.item2key[before])
	self.key2item[key] = item
	self.item2key[item] = key

    def delete(self, item):
	self.w.listboxDeleteItem(self.item2key[item])
	del self.key2item[self.item2key[item]]
	del self.item2key[item]

    def replace(self, text, item):
	key = self.w.listboxInsertItem(text, self.item2key[item])
	self.w.listboxDeleteItem(self.item2key[item])
	del self.key2item[self.item2key[item]]
	self.item2key[item] = key
	self.key2item[key] = item

    def current(self):
	return self.key2item[self.w.listboxGetCurrent()]

    def __init__(self, height, scroll = 0, returnExit = 0, width = 0):
	self.w = _snack.listbox(height, scroll, returnExit)
	self.key2item = {}
	self.item2key = {}
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

    def __init__(self, width, text = "", hidden = 0, scroll = 1, 
		 returnExit = 0):
	self.w = _snack.entry(width, text, hidden, scroll, returnExit)


# Form uses hotkeys
hotkeys = { "F1" : _snack.KEY_F1, "F2" : _snack.KEY_F2, "F3" : _snack.KEY_F3, 
            "F4" : _snack.KEY_F4, "F5" : _snack.KEY_F5, "F6" : _snack.KEY_F6, 
            "F7" : _snack.KEY_F7, "F8" : _snack.KEY_F8, "F9" : _snack.KEY_F9, 
            "F10" : _snack.KEY_F10, "F11" : _snack.KEY_F11, 
            "F12" : _snack.KEY_F12 }

for n in hotkeys.keys():
    hotkeys[hotkeys[n]] = n

class Form:

    def addHotKey(self, keyname):
	self.w.addhotkey(hotkeys[keyname])

    def add(self, widget):
	if widget.__dict__.has_key('hotkeys'):
	    for key in widget.hotkeys.keys():
		self.addHotKey(key)

	if widget.__dict__.has_key('gridmembers'):
	    for w in widget.gridmembers:
		self.add(w)
	elif widget.__dict__.has_key('w'):
	    self.trans[widget.w.key] = widget
	    return self.w.add(widget.w)
	return None

    def run(self):
	(what, which) = self.w.run()
	if (what == _snack.FORM_EXIT_WIDGET):
	    return self.trans[which]

	return hotkeys[which]

    def __init__(self):
	self.trans = {}
	self.w = _snack.form()

class Grid:

    def place(self, x, y):
	return self.g.place(x, y)

    def setField(self, what, col, row, padding = (0, 0, 0, 0),
		 anchorLeft = 0, anchorTop = 0, anchorRight = 0,
		 anchorBottom = 0, growx = 0, growy = 0):
	self.gridmembers.append(what)
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
	self.gridmembers = []

class SnackScreen:

    def __init__(self):
	_snack.init()
	(self.width, self.height) = _snack.size()
	self.pushHelpLine(None)

    def finish(self):
	return _snack.finish()

    def openWindow(self, left, top, width, height, title):
	return _snack.openwindow(left, top, width, height, title)

    def pushHelpLine(self, text):
	if (not text):
	    return _snack.pushhelpline("*default*")
	else:
	    return _snack.pushhelpline(text)

    def popHelpLine(self):
	return _snack.pophelpline()

    def drawRootText(self, left, top, text):
	return _snack.drawroottext(left, top, text)

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

class RadioGroup(Widget):

    def __init__(self):
	self.prev = None
	self.buttonlist = []

    def add(self, title, value, default = None):
	if not self.prev and default == None:
	    # If the first element is not explicitly set to
	    # not be the default, make it be the default
	    default = 1
	b = SingleRadioButton(title, self.prev, default)
	self.prev = b
	self.buttonlist.append((b, value))
	return b

    def getSelection(self):
	for (b, value) in self.buttonlist:
	    if b.selected(): return value
	return None


class RadioBar(Grid):

    def __init__(self, screen, buttonlist):
	self.list = []
	self.item = 0
	self.group = RadioGroup()
	Grid.__init__(self, 1, len(buttonlist))
	for (title, value, default) in buttonlist:
	    b = self.group.add(title, value, default)
	    self.list.append(b, value)
	    self.setField(b, 0, self.item, anchorLeft = 1)
	    self.item = self.item + 1

    def getSelection(self):
	return self.group.getSelection()
	

# you normally want to pack a ButtonBar with growx = 1

class ButtonBar(Grid):

    def __init__(self, screen, buttonlist):
	self.list = []
	self.hotkeys = {}
	self.item = 0
	Grid.__init__(self, len(buttonlist), 1)
	for blist in buttonlist:
	    if len(blist) == 2:
		(title, value) = blist
	    else:
		(title, value, hotkey) = blist
		self.hotkeys[hotkey] = value
	    b = Button(title)
	    self.list.append(b, value)
	    self.setField(b, self.item, 0, (1, 0, 1, 0))
	    self.item = self.item + 1

    def buttonPressed(self, result):
	"""Takes the widget returned by Form.run and looks to see
	if it was one of the widgets in the ButtonBar."""

	if self.hotkeys.has_key(result):
	    return self.hotkeys[result]

	for (button, value) in self.list:
	    if result == button:
		return value
	return None


class GridForm(Grid):

    def __init__(self, screen, title, *args):
	self.screen = screen
	self.title = title
	self.form = Form()
	self.childList = []
	self.form_created = 0
	args = list(args)
	args[:0] = [self]
	apply(Grid.__init__, tuple(args))

    def add(self, widget, col, row, padding = (0, 0, 0, 0),
            anchorLeft = 0, anchorTop = 0, anchorRight = 0,
            anchorBottom = 0, growx = 0, growy = 0):
	self.setField(widget, col, row, padding, anchorLeft,
		      anchorTop, anchorRight, anchorBottom,
		      growx, growy);
	self.childList.append(widget)

    def run_once(self):
	result = self.run()
	self.screen.popWindow()
	return result

    def run(self):
	if not self.form_created:
	    self.place(1,1)
	    for child in self.childList:
		self.form.add(child)
	    self.screen.gridWrappedWindow(self, self.title)
	    self.form_created = 1
	return self.form.run()
	
    def run_popup(self):
	if not self.form_created:
	    self.place(1,1)
	    for child in self.childList:
		self.form.add(child)
	    self.form_created = 1
	self.screen.gridWrappedWindow(self, self.title)
	result = self.form.run()
	self.screen.popWindow()
	return result
