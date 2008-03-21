%{!?python_sitearch: %define python_sitearch %(%{__python} -c "from distutils.sysconfig import get_python_lib; print get_python_lib(1)")}
Summary: A library for text mode user interfaces
Name: newt
Version: 0.52.9
Release: 1%{?dist}
License: LGPLv2
Group: System Environment/Libraries
URL: https://fedorahosted.org/newt/
Source: https://fedorahosted.org/releases/n/e/newt/newt-%{version}.tar.gz
BuildRequires: popt-devel python-devel slang-devel
Provides: snack = %{version}-%{release}
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

%package devel
Summary: Newt windowing toolkit development files
Requires: slang-devel %{name} = %{version}-%{release}
Group: Development/Libraries

# The loader portion of the installer needs to link statically against libnewt,
# so the static library must be shipped.
%package static
Summary: Newt windowing toolkit static library
Requires: %{name}-devel = %{version}-%{release}
Group: Development/Libraries

%Description
Newt is a programming library for color text mode, widget based user
interfaces.  Newt can be used to add stacked windows, entry widgets,
checkboxes, radio buttons, labels, plain text fields, scrollbars,
etc., to text mode user interfaces.  This package also contains the
shared library needed by programs built with newt, as well as a
/usr/bin/dialog replacement called whiptail.  Newt is based on the
slang library.

%description devel
The newt-devel package contains the header files and libraries
necessary for developing applications which use newt.  Newt is a
development library for text mode user interfaces.  Newt is based on
the slang library.

Install newt-devel if you want to develop applications which will use
newt.

%description static
The newt-static package contains the static version of the newt library.
Install it if you need to link statically with libnewt.

%package python
Summary: Python bindings for newt
Group: Development/Libraries
Requires: newt = %{version}-%{release}

%description python
The newt-python package contains the Python bindings for the newt library
providing a python API for creating text mode ionterfaces. 

%prep
%setup -q

%build
# gpm support seems to smash the stack w/ we use help in anaconda??
# --with-gpm-support
%configure --without-tcl
make %{?_smp_mflags} all
chmod 0644 peanuts.py popcorn.py

%install
rm -rf $RPM_BUILD_ROOT
make instroot=$RPM_BUILD_ROOT install

%find_lang %{name}

%clean
rm -rf $RPM_BUILD_ROOT

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files -f %{name}.lang
%defattr (-,root,root)
%doc COPYING
%{_bindir}/whiptail
%{_libdir}/libnewt.so.*
%{_mandir}/man1/whiptail.1*

%files devel
%defattr (-,root,root)
%doc tutorial.sgml
%{_includedir}/newt.h
%{_libdir}/libnewt.so

%files static
%defattr(-,root,root)
%{_libdir}/libnewt.a

%files python
%defattr(-,root,root)
%doc peanuts.py popcorn.py
%{python_sitearch}/*.so
%{python_sitearch}/*.py*

%changelog
* Fri Mar 21 2008 Miroslav Lichvar <mlichvar@redhat.com> - 0.52.9-1
- handle component destruction (patch by Richard W.M. Jones)
- fix newtWinEntry definition
- don't use uninitialized values in newtWinMenu
- remove workarounds for old bug in SLsmg_write_nstring
- improve SIGWINCH handling in form
- don't abort from whiptail gauge on SIGWINCH
- redisplay also last line
- update Polish translation
- update URL and Source tag

* Tue Feb 19 2008 Fedora Release Engineering <rel-eng@fedoraproject.org> - 0.52.8-2
- Autorebuild for GCC 4.3

* Wed Jan 23 2008 Miroslav Lichvar <mlichvar@redhat.com> - 0.52.8-1
- enable slang utf8 mode (#425992)
- support --disable-nls option (patch by Natanael Copa)
- redraw screen when using entry in euc encodings

* Mon Aug 27 2007 Miroslav Lichvar <mlichvar@redhat.com> - 0.52.7-4
- fix segfault in whiptail when no entry is selected in radiolist
- buildrequire popt-devel

* Wed Aug 22 2007 Miroslav Lichvar <mlichvar@redhat.com> - 0.52.7-3
- rebuild

* Wed Aug 08 2007 Miroslav Lichvar <mlichvar@redhat.com> - 0.52.7-2
- add back support for list of Entries in EntryWindow prompts in snack
  (#248878)
- update license tag
- split python module to -python subpackage (patch by Yanko Kaneti)
- fix summary
 
* Fri Jun 15 2007 Miroslav Lichvar <mlichvar@redhat.com> - 0.52.7-1
- add support to snack for multiple selection and border in listbox
  and cursorAtEnd in entry (patch by Shawn Starr)
- fix scrollbar positioning in listbox
- cope with backward system time jumps (#240691)
- free helplines and windows in newtFinished, check for overflow (#239992)
- add release to -devel and -static requires (#238784)

* Thu Apr 12 2007 Miroslav Lichvar <mlichvar@redhat.com> - 0.52.6-3
- fix cursor positioning when setting entry or checkbox flags
- fix counting of items in checkboxtree
- fix some memory leaks

* Wed Apr 04 2007 Miroslav Lichvar <mlichvar@redhat.com> - 0.52.6-2
- fix entry scrolling (#234829)
- fix multibyte character handling in entry

* Fri Mar 02 2007 Miroslav Lichvar <mlichvar@redhat.com> - 0.52.6-1
- add newtSetColor() to allow changing individual colors
- add newtPopWindowNoRefresh() (patch by Forest Bond)
- move static library to -static subpackage, spec cleanup (#226195)
  (patch by Jason Tibbitts)

* Wed Jan 31 2007 Miroslav Lichvar <mlichvar@redhat.com> - 0.52.5-1
- provide option to change text of buttons (#126768)
- don't add escape key to hot keys by default (#216157)
- fix cursor position in checkboxtree, radio button and checkbox
- don't force monochrome terminals to output colors
- highlight active compact button on monochrome terminals
- update translations from debian

* Sat Jan  6 2007 Jeremy Katz <katzj@redhat.com> - 0.52.4-3
- fix memory allocation in snack to be consistent (#212780)

* Thu Dec  7 2006 Jeremy Katz <katzj@redhat.com> - 0.52.4-2
- rebuild for python 2.5

* Fri Oct 13 2006 Miroslav Lichvar <mlichvar@redhat.com> - 0.52.4-1
- fix entry corruption when reading multibyte characters
  and double width character handling
- avoid overflow/crash in scale
- patches from debian
  - fix crash of snack in EntryWindow when prompts is list of tuples
  - put cursor at beginning of text for better accessibility
    in button, scale and textbox
  - add topleft option to whiptail

* Tue Sep 19 2006 Miroslav Lichvar <mlichvar@redhat.com> - 0.52.3-1
- makefile, configure and spec cleanup
- package whiptail.1 and locale files
- fix warnings

* Fri Aug 04 2006 Miroslav Lichvar <mlichvar@redhat.com> - 0.52.2-9
- fix screen corruption when half of double width character is overwritten
  (#137957) 
- fix double width character handling in checkboxtree and listbox

* Wed Jul 12 2006 Jesse Keating <jkeating@redhat.com> - 0.52.2-8.1
- rebuild

* Tue Jun 27 2006 Miroslav Lichvar <mlichvar@redhat.com> - 0.52.2-8
- unfocus when displaying help
- fix help dialog in popcorn.py (#81352)

* Thu Jun 08 2006 Miroslav Lichvar <mlichvar@redhat.com> - 0.52.2-7
- fix checkboxtree positioning
- make textbox with scrollbar focusable (#83203)
- turn off cursor when entry terminated form (#86074)
- handle listbox and checkboxtree focus better (#186053)
- make default colors more friendly to 8-color terminals (#187545)

* Wed May 31 2006 Miroslav Lichvar <mlichvar@redhat.com> - 0.52.2-6.1
- fix handling windows larger than screen size (#189981)

* Fri Feb 10 2006 Jesse Keating <jkeating@redhat.com> - 0.52.2-5.2
- bump again for double-long bug on ppc(64)

* Tue Feb 07 2006 Jesse Keating <jkeating@redhat.com> - 0.52.2-5.1
- rebuilt for new gcc4.1 snapshot and glibc changes

* Tue Jan 17 2006 Petr Rockai <prockai@redhat.com> - 0.52.2-5
- Fix a crash in checkboxtree.c where pressing pgup/pgdown
  on a checkboxtree with less items than its height would
  cause segmentation violation. Consult BR 165347.

* Tue Jan 17 2006 Petr Rockai <prockai@redhat.com> - 0.52.2-4
- Apply patch by Bill Nottingham (thanks) to improve scrollbar appearance
  (BR 174771).
- Add -%%{release} to snack's Provides: line (just in case).

* Tue Jan 17 2006 Petr Rockai <prockai@redhat.com> - 0.52.2-3
- Provide: snack = %%{version} instead of plain "snack", so that
  we don't block upgrades of custom "snack" packages. This should
  not break anything. (Hopefully) fixes BR 171415.

* Mon Jan 16 2006 Petr Rockai <prockai@redhat.com> - 0.52.2-2
- do not build whiptcl, as per 177346 -- so that we avoid dependency on tcl

* Fri Dec 09 2005 Jesse Keating <jkeating@redhat.com>
- rebuilt

* Thu Nov 24 2005 Jindrich Novy <jnovy@redhat.com> - 0.52.2-1
- rebuild because of the new slang-2.0.5

* Fri Nov 22 2005 Petr Rockai <prockai@redhat.com> - 0.52.2-0
- new upstream version (minor fixes for the source tarball
  and build system)

* Fri Sep 30 2005 Petr Rockai <prockai@redhat.com> - 0.52.1-0
- revert bidi patch, objections by Jeremy Katz about
  anaconda breaking
- this version still only exists as a "ghastly" upstream tarball;
  the patches are now cleaned up and will be integrated into
  rhlinux cvs unless some more breakage akin to bidi occurs
- the if1close patch is now part of upstream tarball too

* Wed Sep 21 2005 Petr Rockai <prockai@redhat.com> - 0.52.0-0
- new upstream version

* Fri Sep 02 2005 Petr Rockai <prockai@redhat.com>
- use versioned symbols, patch by Alastair McKinstry, mckinstry at
  debian dot org, thanks
- need private wstrlen due to versioned syms, patch from debian
  package of newt
- both of the above needed to be forward-ported

* Sun Mar 06 2005 Petr Rockai <prockai@redhat.com>
- rebuild

* Mon Nov  8 2004 Jeremy Katz <katzj@redhat.com> - 0.51.6-6
- rebuild for python 2.4

* Fri Oct 15 2004 Adrian Havill <havill@redhat.com> 0.51.6-5
- only do gpmclose if gpmopen succeeed (#118530)

* Thu Oct 14 2004 Adrian Havill <havill@redhat.com> 0.51.6-4
- make the python version dynamic (#114419)

* Tue Jun 15 2004 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Tue Mar 02 2004 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Fri Feb 13 2004 Elliot Lee <sopwith@redhat.com>
- rebuilt

* Thu Nov  6 2003 Jeremy Katz <katzj@redhat.com> 0.51.6-2
- rebuild for python 2.3

* Tue Aug 19 2003 Michael K. Johnson <johnsonm@redhat.com> 0.51.6-1
- rebuild

* Tue Aug 19 2003 Michael K. Johnson <johnsonm@redhat.com> 0.51.5-1
- incorporated listbox cursor location patch (#69903)

* Wed Feb  5 2003 Matt Wilson <msw@redhat.com> 0.51.4-1
- fixed help line drawing in UTF-8 (#81718)
- calculate the width of text in entries using wstrlen
- always set component width to the new label width in newtLabelSetText
- fixed snack.CListbox to work properly with UTF-8 (#81718)

* Tue Feb 04 2003 Florian La Roche <Florian.LaRoche@redhat.de>
- add symlink to shared lib

* Sun Jan  5 2003 Adrian Havill <havill@redhat.com> 0.51.3-1
- cleaned up const qualifiers in interfaces
- added Ctrl-L screen refresh
- fixed segfault in test.c when listbox items are selected
- accessibility: made newt useable with monochrome terms (#60931)
- leave the symbols in the libs (#60400)
- fixed grammar in tutorial (#63496)
- error checking (curcomp exists) for formEvent, newtFormGetCurrent,
  removed fifty button limit (#59027)

* Tue Dec 17 2002 Matt Wilson <msw@redhat.com> 0.51.2-1
- fixed wstrlen() it was calculating wcwidth(first wide char in
  string) * strlen(str) instead of the actual width of the whole
  string
- fixed newtRedrawHelpLine() to copy all the bytes from a multibyte
  string

* Fri Dec 13 2002 Elliot Lee <sopwith@redhat.com> 0.51.1-1
- Merge multilib changes

* Thu Aug 15 2002 Bill Nottingham <notting@redhat.com> 0.51.0-1
- changes for element width calculation for UTF-8
- fix textwrap for UTF-8 in general
- bump soname to avoid shared library collisions with slang

* Wed Jul 01 2002 Michael Fulbright <msf@redhat.com> 0.50.39-1
- Changed a test to check for 'None' the correct way

* Wed Jun 26 2002 Bill Nottingham <notting@redhat.com> 0.50.38-1
- don't hardcode linedrawing characters in the scrollbar code

* Mon Jun 24 2002 Bill Nottingham <notting@redhat.com> 0.50.37-1
- minor tweaks for use with UTF-8 slang

* Tue Jun 11 2002 Joe Orton <jorton@redhat.com> 0.50.36-1
- add newtListboxGetItemCount() API call
- include numeric percentage in scale widget appearace
- add support for ESC key using NEWT_KEY_ESCAPE

* Mon Mar 18 2002 Bill Nottingham <notting@redhat.com> 0.50.35-1
- build for whatever version of python happens to be installed

* Fri Sep 15 2001 Trond Eivind Glomsrød <teg@redhat.com> 0.50.34-1
- remove python2 subpackage
- compile package for python 2.2

* Wed Aug 29 2001 Trond Eivind Glomsrød <teg@redhat.com> 0.50.33-1
- s/Copyright/License/
- Add slang-devel to build dependencies (#49542)

* Wed Aug 22 2001 Crutcher Dunnavant <crutcher@redhat.com> 0.50.32-1
- re-ordered the width key of CheckboxTree.__init__; #52319

* Wed Aug  8 2001 Crutcher Dunnavant <crutcher@redhat.com> 0.50.31-1
- right anchor the internal Listbox of CListboxes, so that empty
- scrollable CListboxes do not look like crape.

* Thu Jul 05 2001 Crutcher Dunnavant <crutcher@redhat.com>
- padded hidden checkboxes on CheckboxTrees

* Thu Jul 05 2001 Crutcher Dunnavant <crutcher@redhat.com>
- taught CheckboxTrees about width. Whohoo! 2-D!!!

* Thu Jul 05 2001 Crutcher Dunnavant <crutcher@redhat.com>
- added 'hide_checkbox' and 'unselectable' options to CheckboxTrees

* Mon Jun 25 2001 Jeremy Katz <katzj@redhat.com>
- CListBox -> CListbox for API consistency
- fixup replace() method of CListbox

* Fri Jun 8 2001 Jeremy Katz <katzj@redhat.com>
- few bugfixes to the CListBox

* Fri Jun 8 2001 Jeremy Katz <katzj@redhat.com>
- added python binding for newtListboxClear() for Listbox and CListBox
- let ButtonBars optionally be made of CompactButtons

* Wed Jun  6 2001 Crutcher Dunnavant <crutcher@redhat.com>
- added CListBox python convenience class

* Tue May 15 2001 Michael Fulbright <msf@redhat.com>
- added python binding for CompactButton()

* Tue Apr  3 2001 Matt Wilson <msw@redhat.com>
- change from using SLsmg_touch_screen to SLsmg_touch_lines to prevent
  excessive flashing due to screen clears when using touch_screen (more
  Japanese handling)

* Mon Apr  2 2001 Matt Wilson <msw@redhat.com>
- redraw the screen in certain situations when LANG=ja_JP.eucJP to
  prevent corrupting kanji characters (#34362)

* Mon Apr  2 2001 Elloit Lee <sopwith@redhat.com>
- Allow python scripts to watch file handles
- Fix 64-bit warnings in snackmodule
- Misc snack.py cleanups
- Add NEWT_FD_EXCEPT to allow watching for fd exceptions
- In newtExitStruct, return the first file descriptor that an event occurred on 

* Fri Mar 30 2001 Matt Wilson <msw@redhat.com>
- don't blow the stack if we push a help line that is longer than the
  curret number of columns
- clip window to screen bounds so that if we get a window that is
  larger than the screen we can still redraw the windows behind it
  when we pop

* Sun Feb 11 2001 Than Ngo <than@redhat.com>
- disable building new-python2 sub package again

* Thu Feb 01 2001 Erik Troan <ewt@redhat.com>
- gave up on separate CHANGES file
- added newtCheckboxTreeSetCurrent() and snack binding
- don't require python2 anymore

* Mon Jan 22 2001 Than Ngo <than@redhat.com>
- don't build newt-python2 sub package.

* Fri Dec 15 2000 Trond Eivind Glomsrød <teg@redhat.com>
- use %%{_tmppath}
- add python2 subpackage, with such support
- fix use of append in snack.py

* Fri Sep 08 2000 Trond Eivind Glomsrød <teg@redhat.com>
- bytecompile the snack python module
- move the libnewt.so symlink to the devel package
- include popcorn.py and peanuts.py in the devel package,
  so we have some documentation of the snack module

* Tue Aug 22 2000 Erik Troan <ewt@redhat.com>
- fixed cursor disappearing in suspend (again)

* Sat Aug 19 2000 Preston Brown <pbrown@redhat.com>
- explicit requirement of devel subpackage on same version of main package
  so that .so file link doesn't break

* Wed Aug 16 2000 Erik Troan <ewt@redhat.com>
- fixed cursor disappearing in suspend
- moved libnewt.so to main package from -devel

* Thu Aug  3 2000 Matt Wilson <msw@redhat.com>
- added setValue method for checkboxes in snack

* Wed Jul 05 2000 Michael Fulbright <msf@redhat.com>
- added NEWT_FLAG_PASSWORD for entering passwords and having asterix echo'd

* Fri Jun 16 2000 Matt Wilson <msw@redhat.com>
- build for new release

* Fri Apr 28 2000 Jakub Jelinek <jakub@redhat.com>
- see CHANGES

* Mon Mar 13 2000 Matt Wilson <msw@redhat.com>
- revert mblen patch, go back to our own wide char detection

* Fri Feb 25 2000 Bill Nottingham <notting@redhat.com>
- fix doReflow to handle mblen returning 0

* Wed Feb 23 2000 Preston Brown <pbrown@redhat.com>
- fix critical bug in fkey 1-4 recognition on xterms

* Wed Feb  9 2000 Matt Wilson <msw@redhat.com>
- fixed snack widget setcallback function

* Thu Feb 03 2000 Erik Troan <ewt@redhat.com>
- strip shared libs

* Mon Jan 31 2000 Matt Wilson <msw@redhat.com>
- added patch from Toru Hoshina <t@kondara.org> to improve multibyte
  character wrapping

* Thu Jan 20 2000 Erik Troan <ewt@redhat.com>
- see CHANGES

* Thu Jan 20 2000 Preston Brown <pbrown@redhat.com>
- fix segfault in newtRadioGetCurrent

* Mon Jan 17 2000 Erik Troan <ewt@redhat.com>
- added numerous bug fixes (see CHANGES)

* Mon Dec 20 1999 Matt Wilson <msw@redhat.com>
- rebuild with fix for listbox from Nalin

* Wed Oct 20 1999 Matt Wilson <msw@redhat.com>
- added patch to correctly wrap euc kanji

* Wed Sep 01 1999 Erik Troan <ewt@redhat.com>
- added suspend/resume to snack

* Tue Aug 31 1999 Matt Wilson <msw@redhat.com>
- enable gpm support

* Fri Aug 27 1999 Matt Wilson <msw@redhat.com>
- added hotkey assignment for gridforms, changed listbox.setcurrent to
  take the item key

* Wed Aug 25 1999 Matt Wilson <msw@redhat.com>
- fixed snack callback function refcounts, as well as optional data args
- fixed suspend callback ref counts

* Mon Aug 23 1999 Matt Wilson <msw@redhat.com>
- added buttons argument to entrywindow

* Thu Aug 12 1999 Bill Nottingham <notting@redhat.com>
- multi-state checkboxtrees. Woohoo.

* Mon Aug  9 1999 Matt Wilson <msw@redhat.com>
- added snack wrappings for checkbox flag setting

* Thu Aug  5 1999 Matt Wilson <msw@redhat.com>
- added snack bindings for setting current listbox selection
- added argument to set default selection in snack ListboxChoiceWindow

* Mon Aug  2 1999 Matt Wilson <msw@redhat.com>
- added checkboxtree
- improved snack binding

* Fri Apr  9 1999 Matt Wilson <msw@redhat.com>
- fixed a glibc related bug in reflow that was truncating all text to 1000
chars

* Fri Apr 09 1999 Matt Wilson <msw@redhat.com>
- fixed bug that made newt apps crash when you hit <insert> followed by lots
of keys

* Mon Mar 15 1999 Matt Wilson <msw@redhat.com>
- fix from Jakub Jelinek for listbox keypresses

* Fri Feb 27 1999 Matt Wilson <msw@redhat.com>
- fixed support for navigating listboxes with alphabetical keypresses

* Thu Feb 25 1999 Matt Wilson <msw@redhat.com>
- updated descriptions
- added support for navigating listboxes with alphabetical keypresses

* Mon Feb  8 1999 Matt Wilson <msw@redhat.com>
- made grid wrapped windows at least the size of their title bars

* Fri Feb  5 1999 Matt Wilson <msw@redhat.com>
- Function to set checkbox flags.  This will go away later when I have
  a generic flag setting function and signals to comps to go insensitive.

* Tue Jan 19 1999 Matt Wilson <msw@redhat.com>
- Stopped using libgpm, internalized all gpm calls.  Still need some cleanups.

* Thu Jan  7 1999 Matt Wilson <msw@redhat.com>
- Added GPM mouse support
- Moved to autoconf to allow compiling without GPM support
- Changed revision to 0.40

* Wed Oct 21 1998 Bill Nottingham <notting@redhat.com>
- built against slang-1.2.2

* Wed Aug 19 1998 Bill Nottingham <notting@redhat.com>
- bugfixes for text reflow
- added docs

* Fri May 01 1998 Cristian Gafton <gafton@redhat.com>
- devel package moved to Development/Libraries

* Thu Apr 30 1998 Erik Troan <ewt@redhat.com>
- removed whiptcl.so -- it should be in a separate package

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
