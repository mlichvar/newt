# snack.py: maps C extension module _snack to proper python types in module
# snack.
# The first section is a very literal mapping.
# The second section contains convenience classes that amalgamate
# the literal classes and make them more object-oriented.

import _snack
import types
import string

from _snack import FLAG_DISABLED, FLAGS_SET, FLAGS_RESET, FLAGS_TOGGLE, FD_READ, FD_WRITE, FD_EXCEPT

LEFT = (-1, 0)
DOWN = (-1, -1)
CENTER = (0, 0)
UP = (1, 1)
RIGHT = (1, 0)

snackArgs = {"append":-1}

class Widget:
    def setCallback(self, obj, data = None):
        if data:
            self.w.setCallback(obj, data)
        else:
            self.w.setCallback(obj)
            
    def __init__(self):
	self.w = None

class Button(Widget):

    def __init__(self, text):
	self.w = _snack.button(text)

class CompactButton(Widget):

    def __init__(self, text):
	self.w = _snack.compactbutton(text)

class Checkbox(Widget):

    def value(self):
	return self.w.checkboxValue

    def selected(self):
	return self.w.checkboxValue != 0

    def setFlags (self, flag, sense):
        return self.w.checkboxSetFlags(flag, sense)

    def setValue (self, value):
        return self.w.checkboxSetValue(value)

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

    def setCurrent(self, item):
	self.w.listboxSetCurrent(self.item2key[item])

    def clear(self):
        self.key2item = {}
        self.item2key = {}        
        self.w.listboxClear()

    def __init__(self, height, scroll = 0, returnExit = 0, width = 0, showCursor = 0):
	self.w = _snack.listbox(height, scroll, returnExit, showCursor)
	self.key2item = {}
	self.item2key = {}
	if (width):
	    self.w.listboxSetWidth(width)

class Textbox(Widget):

    def setText(self, text):
	self.w.textboxText(text)

    def __init__(self, width, height, text, scroll = 0, wrap = 0):
	self.w = _snack.textbox(width, height, text, scroll, wrap)

class TextboxReflowed(Textbox):

    def __init__(self, width, text, flexDown = 5, flexUp = 10, maxHeight = -1):
	(newtext, width, height) = reflow(text, width, flexDown, flexUp)
        if maxHeight != -1 and height > maxHeight:
            Textbox.__init__(self, width, maxHeight, newtext, 1)
        else:
            Textbox.__init__(self, width, height, newtext, 0)

class Label(Widget):

    def setText(self, text):
	self.w.labelText(text)

    def __init__(self, text):
	self.w = _snack.label(text)

class Scale(Widget):

    def set(self, amount):
	self.w.scaleSet(amount)

    def __init__(self, width, total):
	self.w = _snack.scale(width, total)

class Entry(Widget):

    def value(self):
	return self.w.entryValue

    def set(self, text):
	return self.w.entrySetValue(text)

    def setFlags (self, flag, sense):
        return self.w.entrySetFlags(flag, sense)

    def __init__(self, width, text = "", hidden = 0, password = 0, scroll = 1, 
		 returnExit = 0):
	self.w = _snack.entry(width, text, hidden, password, scroll, returnExit)


# Form uses hotkeys
hotkeys = { "F1" : _snack.KEY_F1, "F2" : _snack.KEY_F2, "F3" : _snack.KEY_F3, 
            "F4" : _snack.KEY_F4, "F5" : _snack.KEY_F5, "F6" : _snack.KEY_F6, 
            "F7" : _snack.KEY_F7, "F8" : _snack.KEY_F8, "F9" : _snack.KEY_F9, 
            "F10" : _snack.KEY_F10, "F11" : _snack.KEY_F11, 
            "F12" : _snack.KEY_F12, " " : ord(" ") }

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
	elif (what == _snack.FORM_EXIT_TIMER):
	    return "TIMER"
	elif (what == _snack.FORM_EXIT_FDREADY):
	    return self.filemap[which]

	return hotkeys[which]

    def draw(self):
	self.w.draw()
	return None

    def __init__(self, helpArg = None):
	self.trans = {}
	self.filemap = {}
	self.w = _snack.form(helpArg)
	# we do the reference count for the helpArg in python! gross
	self.helpArg = helpArg

    def setCurrent (self, co):
        self.w.setcurrent (co.w)

    def setTimer (self, timer):
        self.w.settimer (timer)

    def watchFile (self, file, flags):
	self.filemap[file.fileno()] = file
	self.w.watchfd (file.fileno(), flags)

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

    def resume(self):
	_snack.resume()

    def suspend(self):
	_snack.suspend()

    def doHelpCallback(self, arg):
	self.helpCb(self, arg)

    def helpCallback(self, cb):
	self.helpCb = cb
	return _snack.helpcallback(self.doHelpCallback)

    def suspendCallback(self, cb, data = None):
        if data:
            return _snack.suspendcallback(cb, data)
        return _snack.suspendcallback(cb)

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

    def gridWrappedWindow(self, grid, title, x = None, y = None):
	if x and y:
	    return _snack.gridwrappedwindow(grid.g, title, x, y)

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
	    self.list.append((b, value))
	    self.setField(b, 0, self.item, anchorLeft = 1)
	    self.item = self.item + 1

    def getSelection(self):
	return self.group.getSelection()
	

# you normally want to pack a ButtonBar with growx = 1

class ButtonBar(Grid):

    def __init__(self, screen, buttonlist, compact = 0):
	self.list = []
	self.hotkeys = {}
	self.item = 0
	Grid.__init__(self, len(buttonlist), 1)
	for blist in buttonlist:
	    if (type(blist) == types.StringType):
		title = blist
		value = string.lower(blist)
	    elif len(blist) == 2:
		(title, value) = blist
	    else:
		(title, value, hotkey) = blist
		self.hotkeys[hotkey] = value

            if compact:
                b = CompactButton(title)
            else:
                b = Button(title)
	    self.list.append((b, value))
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


class GridFormHelp(Grid):

    def __init__(self, screen, title, help, *args):
	self.screen = screen
	self.title = title
	self.form = Form(help)
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

    def runOnce(self, x = None, y = None):
	result = self.run(x, y)
	self.screen.popWindow()
	return result

    def addHotKey(self, keyname):
	self.form.addHotKey(keyname)

    def setTimer(self, keyname):
	self.form.setTimer(keyname)

    def create(self, x = None, y = None):
	if not self.form_created:
	    self.place(1,1)
	    for child in self.childList:
		self.form.add(child)
	    self.screen.gridWrappedWindow(self, self.title, x, y)
	    self.form_created = 1

    def run(self, x = None, y = None):
	self.create(x, y)
	return self.form.run()

    def draw(self):
	self.create()
	return self.form.draw()
	
    def runPopup(self):
	self.create()
	self.screen.gridWrappedWindow(self, self.title)
	result = self.form.run()
	self.screen.popWindow()
	return result

    def setCurrent (self, co):
        self.form.setCurrent (co)

class GridForm(GridFormHelp):

    def __init__(self, screen, title, *args):
	myargs = (self, screen, title, None) + args
	apply(GridFormHelp.__init__, myargs)

class CheckboxTree(Widget):
    def append(self, text, item = None, selected = 0):
    	self.addItem(text, (snackArgs['append'], ), item, selected)

    def addItem(self, text, path, item = None, selected = 0):
    	if item is None:
	    item = text
	key = self.w.checkboxtreeAddItem(text, path, selected)
	self.key2item[key] = item
	self.item2key[item] = key

    def getCurrent(self):
	curr = self.w.checkboxtreeGetCurrent()
	return self.key2item[curr]

    def __init__(self, height, scroll = 0, width = None, hide_checkbox = 0, unselectable = 0):
	self.w = _snack.checkboxtree(height, scroll, hide_checkbox, unselectable)
	self.key2item = {}
	self.item2key = {}
	if (width):
	    self.w.checkboxtreeSetWidth(width)

    def getSelection(self):
        selection = []
        list = self.w.checkboxtreeGetSelection()
        for key in list:
            selection.append(self.key2item[key])
	return selection

    def setEntry(self, item, text):
	self.w.checkboxtreeSetEntry(self.item2key[item], text)

    def setCurrent(self, item):
	self.w.checkboxtreeSetCurrent(self.item2key[item])

    def setEntryValue(self, item, selected = 1):
	self.w.checkboxtreeSetEntryValue(self.item2key[item], selected)

    def getEntryValue(self, item):
	return self.w.checkboxtreeGetEntryValue(self.item2key[item])

def ListboxChoiceWindow(screen, title, text, items, 
			buttons = ('Ok', 'Cancel'), 
			width = 40, scroll = 0, height = -1, default = None,
			help = None):
    if (height == -1): height = len(items)

    bb = ButtonBar(screen, buttons)
    t = TextboxReflowed(width, text)
    l = Listbox(height, scroll = scroll, returnExit = 1)
    count = 0
    for item in items:
	if (type(item) == types.TupleType):
	    (text, key) = item
	else:
	    text = item
	    key = count

	if (default == count):
	    default = key
	elif (default == item):
	    default = key

	l.append(text, key)
	count = count + 1

    if (default != None):
	l.setCurrent (default)

    g = GridFormHelp(screen, title, help, 1, 3)
    g.add(t, 0, 0)
    g.add(l, 0, 1, padding = (0, 1, 0, 1))
    g.add(bb, 0, 2, growx = 1)

    rc = g.runOnce()

    return (bb.buttonPressed(rc), l.current())

def ButtonChoiceWindow(screen, title, text, 
		       buttons = [ 'Ok', 'Cancel' ], 
		       width = 40, x = None, y = None, help = None):
    bb = ButtonBar(screen, buttons)
    t = TextboxReflowed(width, text, maxHeight = screen.height - 12)

    g = GridFormHelp(screen, title, help, 1, 2)
    g.add(t, 0, 0, padding = (0, 0, 0, 1))
    g.add(bb, 0, 1, growx = 1)
    return bb.buttonPressed(g.runOnce(x, y))

def EntryWindow(screen, title, text, prompts, allowCancel = 1, width = 40,
		entryWidth = 20, buttons = [ 'Ok', 'Cancel' ], help = None):
    bb = ButtonBar(screen, buttons);
    t = TextboxReflowed(width, text)

    count = 0
    for n in prompts:
	count = count + 1

    sg = Grid(2, count)

    count = 0
    entryList = []
    for n in prompts:
	if (type(n) == types.TupleType):
	    (n, e) = n
	else:
	    e = Entry(entryWidth)

	sg.setField(Label(n), 0, count, padding = (0, 0, 1, 0), anchorLeft = 1)
	sg.setField(e, 1, count, anchorLeft = 1)
	count = count + 1
	entryList.append(e)

    g = GridFormHelp(screen, title, help, 1, 3)

    g.add(t, 0, 0, padding = (0, 0, 0, 1))
    g.add(sg, 0, 1, padding = (0, 0, 0, 1))
    g.add(bb, 0, 2, growx = 1)

    result = g.runOnce()

    entryValues = []
    count = 0
    for n in prompts:
	entryValues.append(entryList[count].value())
	count = count + 1

    return (bb.buttonPressed(result), tuple(entryValues))

class CListbox(Grid):
        def __init__(self, height, cols, col_widths, scroll = 0,
                     returnExit = 0, width = 0, col_pad = 1,
                     col_text_align = None, col_labels = None,
                     col_label_align = None, adjust_width=0):

		self.cols = cols
		self.col_widths = col_widths[:]
		self.col_pad = col_pad
		self.col_text_align = col_text_align

		if col_labels != None:		
			Grid.__init__(self, 1, 2)
			box_y = 1

			lstr = self.colFormText(col_labels, col_label_align,
                                                adjust_width=adjust_width)
			self.label = Label(lstr)
			self.setField(self.label, 0, 0, anchorLeft=1)

		else:
			Grid.__init__(self, 1, 1)
			box_y = 0
			

		self.listbox = Listbox(height, scroll, returnExit, width)
		self.setField(self.listbox, 0, box_y, anchorRight=1)

	def colFormText(self, col_text, align = None, adjust_width=0):
		i = 0
		str = ""
		c_len = len(col_text)
		while (i < self.cols) and (i < c_len):
		
			cstr = col_text[i]
                        cstrlen = _snack.wstrlen(cstr)
                        if self.col_widths[i] < cstrlen:
                            if adjust_width:
                                self.col_widths[i] = cstrlen
                            else:
                                cstr = cstr[:self.col_widths[i]]

			delta = self.col_widths[i] - _snack.wstrlen(cstr)
                        
			if delta > 0:
				if align == None:
                                    a = LEFT
                                else:
                                    a = align[i]

				if a == LEFT:
					cstr = cstr + (" " * delta)
				if a == CENTER:
					cstr = (" " * (delta / 2)) + cstr + \
						(" " * ((delta + 1) / 2))
				if a == RIGHT:
					cstr = (" " * delta) + cstr

			if i != c_len - 1:
				pstr = (" " * self.col_pad)
			else:
				pstr = ""

			str = str + cstr + pstr
	
			i = i + 1
	
		return str

	def append(self, col_text, item, col_text_align = None):
		if col_text_align == None:
			col_text_align = self.col_text_align
		text = self.colFormText(col_text, col_text_align)
		self.listbox.append(text, item)

	def insert(self, col_text, item, before, col_text_align = None):
		if col_text_align == None:
			col_text_align = self.col_text_align
		text = self.colFormText(col_text, col_text_align)
		self.listbox.insert(text, item, before)

	def delete(self, item):
		self.listbox.delete(item)

	def replace(self, col_text, item, col_text_align = None):
		if col_text_align == None:
			col_text_align = self.col_text_align
		text = self.colFormText(col_text, col_text_align)
		self.listbox.replace(text, item)

	def current(self):
		return self.listbox.current()

	def setCurrent(self, item):
		self.listbox.setCurrent(item)

        def clear(self):
            self.listbox.clear()
