Summary: Not Erik's Windowing Toolkit - text mode windowing with slang
Name: newt
%define version 0.21
Version: %{version}
Release: 1
Copyright: LGPL
Group: Libraries
Source: ftp://ftp.redhat.com/pub/redhat/code/newt/newt-%{version}.tar.gz
Requires: slang
Provides: snack
%package devel
Summary: Developer's toolkit for newt windowing library
Requires: slang-devel
Group: Libraries
BuildRoot: /var/tmp/newtroot

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
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make instroot=$RPM_BUILD_ROOT install
make instroot=$RPM_BUILD_ROOT install-sh

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%changelog

* Mon Feb 16 1998 Erik Troan <ewt@redhat.com>

- added newtWinMenu()
- many bug fixes in grid code

* Wed Jan 21 1998 Erik Troan <ewt@redhat.com>

- removed newtWinTernary()
- made newtWinChoice() return codes consistent with newtWinTernary()

* Fri Jan 16 1998 Erik Troan <ewt@redhat.com>

- added changes from Bruce Perens
    - small cleanups
    - lets whiptail automatically resize windows
- the order of placing a grid and adding components to a form no longer
  matters
- added newtGridAddComponentsToForm()

* Wed Oct 08 1997 Erik Troan <ewt@redhat.com>

- added newtWinTernary()

* Tue Oct 07 1997 Erik Troan <ewt@redhat.com>

- made Make/spec files use a buildroot
- added grid support (for newt 0.11 actually)

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
/usr/lib/whiptcl.so
/usr/lib/python1.4/snack.py
/usr/lib/python1.4/linux-%{buildarch}/_snackmodule.so

%files devel
/usr/include/newt.h
/usr/lib/libnewt.a
/usr/lib/libnewt.so
