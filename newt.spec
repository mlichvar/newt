Summary: Not Erik's Windowing Toolkit - text mode windowing with slang
Name: newt
Version: 0.9
Release: 1
Copyright: LGPL
Group: Libraries
Source: ftp://ftp.redhat.com/pub/redhat/code/newt/newt-0.9.tar.gz
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
for programs that have been built with newt as well as a /usr/bin/dialog
replacement called whiptail.

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
rm -rf /usr/lib/libnewt*
make install
make install-sh

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%changelog

* Mon Jun 02 1997 Erik Troan <ewt@redhat.com>

- Added patched from Clarence Smith for setting the size of a listbox
- Version 0.9

* Tue May 28 1997 Elliot Lee <sopwith@redhat.com> 0.8-2
- Touchups on Makefile
- Cleaned up NEWT_FLAGS_*

* Tue Mar 18 1997 Erik Troan <ewt@redhat.com>

- Cleaned up listbox
- Added whiptail
- Added newtButtonCompact button type and associated colors
- Added newtTextboxGetNumLines() and newtTextboxSetHeight()

* Tue Feb 25 1997 Erik Troan <ewt@redhat.com>

- Added changes from sopwith for C++ cleanliness and some listbox fixes.

%files
/usr/lib/libnewt.so.*
/usr/bin/whiptail

%files devel
/usr/include/newt.h
/usr/lib/libnewt.a
/usr/lib/libnewt.so
