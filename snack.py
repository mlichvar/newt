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
	return self.w.radiobuttonkey == self.w.radioValue;
    
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

    def add(self, widget):
	return self.g.add(widget.w)

    def run(self):
	return self.g.run()

    def __init__(self):
	self.g = _snack.form()

class Grid:

    def place(self, x, y):
	return self.g.place(x, y)

    def setField(self, col, row, what, padding = (0, 0, 0, 0),
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

