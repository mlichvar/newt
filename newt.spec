Summary: Not Erik's Windowing Toolkit - text mode windowing with slang
Name: newt
Version: 0.8
Release: 1
Copyright: LGPL
Group: Libraries
Source: ftp://ftp.redhat.com/pub/redhat/code/newt/newt-0.8.tar.gz
Requires: slang
%package devel
Summary: Developer's toolkit for newt windowing library
Requires: slang-devel
Group: Libraries

%description
Newt is a windowing toolkit for text mode built from the slang library. It
allows color text mode applications to easily use stackable windows, push
buttons, check boxes, radio buttons, lists, entry fields, labels, and
displayable text. Scrollbars are supported, and forms may be nested to
provide extra functionality. This pacakge contains the shared library
for programs that have been built with newt.

%description devel
These are the header files and libraries for developing applications which
use newt. Newt is a windowing toolkit for text mode, which provides many
widgets and stackable windows.

%prep
%setup

%build
make
make shared

%install
make install
make install-sh

%post
/sbin/ldconfig

%postun
/sbin/ldconfig

%changelog

* Tue Feb 25 1997 Erik Troan <ewt@redhat.com>

Added changes from sopwith for C++ cleanliness and some listbox fixes.

%files
/usr/lib/libnewt.so.0.7

%files devel
/usr/include/newt.h
/usr/lib/libnewt.a
/usr/lib/libnewt.so
