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

    def setField(self, col, row, what, padding = (0, 0, 0, 0)):
	return self.g.setfield(col, row, what.w, padding)

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
