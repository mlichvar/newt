#!/usr/bin/python

# Demo program to show use of python-newt module

from __future__ import absolute_import, print_function, unicode_literals
from snack import *

screen = SnackScreen()

li = Listbox(height = 3, width = 20, returnExit = 1)
li.append("First", 1)
li.append("Second", 2)
li.append("Third", 3)
rb = RadioBar(screen, (("This", "this", 0),
			("Default", "default", 1),
			("That", "that", 0)))
bb = ButtonBar(screen, (("Ok", "ok"), ("Cancel", "cancel")))

ct = CheckboxTree(height = 5, scroll = 1)
ct.append("Colors")
ct.addItem("Red", (0, snackArgs['append']))
ct.addItem("Yellow", (0, snackArgs['append']))
ct.addItem("Blue", (0, snackArgs['append']))
ct.append("Flavors")
ct.append("Numbers")
ct.addItem("1", (2, snackArgs['append']))
ct.addItem("2", (2, snackArgs['append']))
ct.addItem("3", (2, snackArgs['append']))
ct.append("Names")
ct.append("Months")
ct.append("Events")
g = GridForm(screen, "My Test", 1, 4)
g.add(li, 0, 0)
g.add(rb, 0, 1, (0, 1, 0, 1))
g.add(ct, 0, 2)
g.add(bb, 0, 3, growx = 1)

result = g.runOnce()

screen.finish()

print(result)
print("listbox:", li.current())
print("rb:", rb.getSelection())
print("bb:", bb.buttonPressed(result))
print("checkboxtree:", ct.getSelection())
