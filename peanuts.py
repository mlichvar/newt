#!/usr/bin/python

from snack import *

screen = SnackScreen()

li = Listbox(4, width = 20, returnExit = 1)
li.append("First")
li.append("Second")
li.append("Third")
rb = RadioBar(screen, (("This", "this", 0),
			("Default", "default", 1),
			("That", "that", 0)))
bb = ButtonBar(screen, (("Ok", "ok"), ("Cancel", "cancel")))

g = GridForm(screen, "My Test", 1, 3)
g.add(li, 0, 0)
g.add(rb, 0, 1)
g.add(bb, 0, 2, growx = 1)

result = g.run()

screen.finish()

print "listbox", li.current()
print "rb", rb.getSelection()
print "bb", bb.buttonPressed(res)
print res
