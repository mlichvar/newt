#!/usr/bin/python

from snack import *

screen = SnackScreen()

b = Button("Button")
e = Entry(15, "Entry")
l = Label("label")
cb = Checkbox("checkbox")
r1 = SingleRadioButton("Radio 1", None, 1)
r2 = SingleRadioButton("Radio 2", r1)

g = Grid(2, 3)
g.setField(0, 0, b)
g.setField(1, 0, e, (1, 0, 0, 0))
g.setField(0, 1, l, (0, 1, 0, 0))
g.setField(1, 1, cb, (1, 1, 0, 0))
g.setField(0, 2, r1, (0, 1, 0, 0))
g.setField(1, 2, r2, (1, 1, 0, 0))
g.place(1, 1)

screen.gridWrappedWindow(g, "title")

f = Form()
f.add(b)
f.add(e)
f.add(l)
f.add(cb)
f.add(r1)
f.add(r2)

f.run()

screen.popWindow()

screen.finish()

print "val", e.value()
print "check", cb.value()

print "r1", r1.selected()
