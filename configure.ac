AC_INIT([newt_pr.h])

PACKAGE=newt
VERSION=0.52.18
SONAME=0.52

AC_CONFIG_HEADER([config.h])

AC_DEFINE_UNQUOTED([PACKAGE], ["$PACKAGE"], [Package Name])
AC_DEFINE_UNQUOTED([VERSION], ["$VERSION"], [NEWT Version String])
AC_SUBST([VERSION])
AC_SUBST([SONAME])
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_GREP
AC_SYS_LARGEFILE

AC_CHECK_SIZEOF([long])
AC_CHECK_SIZEOF([long long])
AC_CHECK_SIZEOF([void *])

# Are we using GNU ld?
AC_MSG_CHECKING([for GNU ld])
LD=`$CC -print-prog-name=ld 2>&5`

if test `$LD -v 2>&1 | $ac_cv_path_GREP -c "GNU ld"` = 0; then
  # Not
  GNU_LD=""
  AC_MSG_RESULT([no])
else
  GNU_LD="$LD"
  AC_MSG_RESULT([yes])
  AC_SUBST([GNU_LD])
fi

AC_CHECK_HEADERS([sys/select.h alloca.h])
AC_CHECK_HEADER([slang.h], [], [
  old_CPPFLAGS=${CPPFLAGS}
  CPPFLAGS="${CPPFLAGS} -I/opt/local/include"
  unset ac_cv_header_slang_h
  AC_CHECK_HEADER([slang.h])
  if test x$ac_cv_header_slang_h = x; then
   CPPFLAGS="${old_CPPFLAGS}"
  else
   LDFLAGS="${LDFLAGS} -L/opt/local/lib"
  fi
])
if test x$ac_cv_header_slang_h = x; then
  old_CPPFLAGS=${CPPFLAGS}
  CPPFLAGS="${CPPFLAGS} -I/sw/include"
  AC_CHECK_HEADER([slang.h])
  if test x$ac_cv_header_slang_h = x; then
    CPPFLAGS=${old_CPPFLAGS}
  else
    LDFLAGS="${LDFLAGS} -L/sw/lib"
  fi
fi
AC_CHECK_HEADERS([popt.h libintl.h])

AC_MSG_CHECKING([for python versions])
AC_ARG_WITH([python], [  --without-python        do not compile python support])
if test "x$with_python" = "xno"; then
  AC_MSG_RESULT([skipped])
  PYTHONVERS=
else
  PYTHONVERS=$(ls /usr/include/python*/Python.h 2> /dev/null | sed 's|.*\(python[[0-9]]*\.[[0-9]]*\).*|\1|g' | tr '\n' ' ')
  AC_MSG_RESULT([$PYTHONVERS])
fi
AC_SUBST([PYTHONVERS])

AC_ARG_WITH([tcl], [  --without-tcl           do not compile whiptcl.so])
if test "x$with_tcl" = "xno"; then
  WHIPTCLLIB=
else
  AC_MSG_CHECKING([for tcl configuration])

  # check in a few common install locations
  if test x"${ac_cv_c_tclconfig}" = x ; then
    for i in `ls -d ${libdir} 2>/dev/null` \
      `ls -d /usr/lib 2>/dev/null` \
      `ls -d /usr/lib64 2>/dev/null` \
      `ls -d /usr/local/lib 2>/dev/null` \
      `ls -d /usr/contrib/lib 2>/dev/null` \
      `ls -d /usr/pkg/lib 2>/dev/null` \
      `ls -d /usr/local/lib/tcl8.5 2>/dev/null` \
      `ls -d /usr/local/lib/tcl8.4 2>/dev/null` \
      `ls -d /usr/lib/tcl8.5 2>/dev/null` \
      `ls -d /usr/lib/tcl8.4 2>/dev/null` \
      `ls -d /opt/local/lib 2>/dev/null` \
      ; do
      if test -f "$i/tclConfig.sh" ; then
        ac_cv_c_tclconfig=`(cd $i; pwd)`
        break
      fi
    done
  fi

  if test x"${ac_cv_c_tclconfig}" != x ; then
    AC_MSG_RESULT([found])
    . ${ac_cv_c_tclconfig}/tclConfig.sh
    AC_SUBST([TCL_LIB_FLAG])
    WHIPTCLLIB=whiptcl
    # needed with tcl8.6
    CPPFLAGS="$CPPFLAGS -DUSE_INTERP_RESULT"
  else
    AC_MSG_RESULT([NOT found])
  fi
fi
AC_SUBST([WHIPTCLLIB])

AC_ARG_WITH([gpm-support], [  --with-gpm-support      compile with GPM support])

if test "x$with_gpm_support" = "xyes"; then
  AC_DEFINE([USE_GPM], 1, [Define to 1 if GPM support is enabled])
fi

AC_ARG_ENABLE([nls], [  --disable-nls           compile without NLS support])

if test "x$enable_nls" != "xno"; then
  AC_CHECK_LIB([c], [gettext], [ ], [
    AC_CHECK_LIB([intl], [gettext], [
      LIBS="-lintl"
      AC_SUBST([LIBS])], [
        if test "x$enable_nls" != "xcheck"; then
          AC_MSG_FAILURE([--enable-nls was specified, but the configure check failed])
        else
          enable_nls=no
        fi
    ])
  ])
  if test "x$enable_nls" != "xno"; then
    AC_DEFINE([ENABLE_NLS], 1, [Define to 1 if NLS support is enabled])
  fi
fi

AC_ARG_WITH(colorsfile, [  --with-colorsfile=file  set default location of colors file])

if test "x$with_colorsfile" != "xno"; then
  if test "x$with_colorsfile" != "xyes"; then
    if test "x$with_colorsfile" != "x"; then
      AC_DEFINE_UNQUOTED(NEWT_COLORS_FILE, "$with_colorsfile", [Default NEWT_COLORS_FILE ])
    fi
  fi
fi

AC_CONFIG_FILES([Makefile libnewt.pc])
AC_OUTPUT

