#!/usr/bin/python

from snack import *

t = Textbox(25, 1, "Some text")
li = Listbox(5, width = 20, returnExit = 1)
li.append("First", "f")
li.append("Second", "s")
li.insert("Another", "a", "f")
li.delete("a")
b = Button("Button")
e = Entry(15, "Entry")
l = Label("label")
cb = Checkbox("checkbox")
r1 = SingleRadioButton("Radio 1", None, 1)
r2 = SingleRadioButton("Radio 2", r1)

screen = SnackScreen()

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

f = Form()
f.add(li)
f.add(b)
f.add(e)
f.add(l)
f.add(cb)
f.add(r1)
f.add(r2)
f.add(t)

f.addHotKey("F1")

res = f.run()

screen.popWindow()

screen.finish()

print "val", e.value()
print "check", cb.value()
print "r1", r1.selected()
print "listbox", li.current()
# returns a tuple of the wrapped text, the actual width, and the actual height
print res
