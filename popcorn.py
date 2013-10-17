#!/usr/bin/python

from __future__ import absolute_import, print_function, unicode_literals
from snack import *
import sys

def help(screen, text):
    ButtonChoiceWindow(screen, "Help", text, help = "Help on help")

t = TextboxReflowed(25, "Some text which needs to be wrapped at a good place.")
li = Listbox(5, width = 20, returnExit = 1)
li.append("First", "f")
li.append("Second", "s")
li.insert("Another", "a", "f")
li.delete("a")
ct = CheckboxTree(5, scroll = 1)
ct.append("Colors")
ct.addItem("Red", (0, snackArgs['append']), "red item key")
ct.addItem("Yellow", (0, snackArgs['append']))
ct.addItem("Blue", (0, snackArgs['append']))
ct.append("Flavors")
ct.addItem("Vanilla", (1, snackArgs['append']))
ct.addItem("Chocolate", (1, snackArgs['append']))
ct.addItem("Stawberry", (1, snackArgs['append']))
ct.append("Numbers")
ct.addItem("1", (2, snackArgs['append']))
ct.addItem("2", (2, snackArgs['append']))
ct.addItem("3", (2, snackArgs['append']))
ct.append("Names")
ct.addItem("Matt", (3, snackArgs['append']))
ct.addItem("Shawn", (3, snackArgs['append']))
ct.addItem("Wilson", (3, snackArgs['append']))
ct.append("Months")
ct.addItem("February", (4, snackArgs['append']))
ct.addItem("August", (4, snackArgs['append']))
ct.addItem("September", (4, snackArgs['append']))
ct.append("Events")
ct.addItem("Christmas", (5, snackArgs['append']))
ct.addItem("Labor Day", (5, snackArgs['append']))
ct.addItem("My Vacation", (5, snackArgs['append']))
b = Button("Button")
e = Entry(15, "Entry")
l = Label("label")
cb = Checkbox("checkbox")
r1 = SingleRadioButton("Radio 1", None, 1)
r2 = SingleRadioButton("Radio 2", r1)

def something():
    print(hello)

screen = SnackScreen()

screen.helpCallback(help)

foo = EntryWindow(screen, 'Title', 'This is some text for the entry window',
	    ['prompt', 'more', 'info'])

lbcw = ListboxChoiceWindow(screen, 'Title 2', 
		    'Choose one item from the list below:', 
		    ('One', 'Two', 'Three', 'Four', 'Five'), default = 2,
		    help = "Help for a listbox")

sg = Grid(2, 3)
sg.setField(b, 0, 0, anchorLeft = 1)
sg.setField(e, 1, 0, (1, 0, 0, 0), anchorLeft = 1, anchorTop = 1)
sg.setField(l, 0, 1, (0, 1, 0, 0), anchorLeft = 1)
sg.setField(cb, 1, 1, (1, 1, 0, 0), anchorLeft = 1)
sg.setField(r1, 0, 2, (0, 0, 0, 0), anchorLeft = 1)
sg.setField(r2, 1, 2, (1, 0, 0, 0), anchorLeft = 1)

g = Grid(1, 3)

g.setField(t, 0, 0)
g.setField(li, 0, 1, (0, 1, 0, 1))
g.setField(sg, 0, 2)

g.place(1, 1)

screen.gridWrappedWindow(g, "title")

f = Form("This is some help")
f.add(li)
f.add(b)
f.add(e)
f.add(l)
f.add(cb)
f.add(r1)
f.add(r2)
f.add(t)

res = f.run()

screen.popWindow()

g = GridForm(screen, "Tree", 1, 2)
g.add(ct, 0, 0, (0, 0, 0, 1))
g.add(Button("Ok"), 0, 1)
g.runOnce()


screen.finish()

print("val", e.value())
print("check", cb.value())
print("r1", r1.selected())
print("listbox", li.current())
# returns a tuple of the wrapped text, the actual width, and the actual height
print(res)

print(foo)
print('lbcw', lbcw)
print("ct selected", ct.getSelection())
print("ct current", ct.getCurrent())
