#!/bin/sh

autoheader
autoconf
automake --foreign -a -c -f
make -C po
