AC_INIT([newt_pr.h])

PACKAGE=newt
VERSION=$(awk '/^Version: / {print $2}' $srcdir/newt.spec)
SONAME=0.52

AC_CONFIG_HEADER([config.h])

AC_DEFINE_UNQUOTED([PACKAGE], ["$PACKAGE"], [Package Name])
AC_DEFINE_UNQUOTED([VERSION], ["$VERSION"], [NEWT Version String])
AC_SUBST([VERSION])
AC_SUBST([SONAME])
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LN_S

AC_CHECK_HEADERS([sys/select.h alloca.h])
AC_CHECK_HEADERS([slang.h])

AC_MSG_CHECKING([for python versions])
PYTHONVERS=$(echo /usr/include/python*/Python.h | sed "s|/usr/include/||g"| sed "s|/Python.h||g" 2> /dev/null)
AC_MSG_RESULT([$PYTHONVERS])
AC_SUBST([PYTHONVERS])

AC_ARG_WITH([tcl], [  --without-tcl           do not compile whiptcl.so])
if test "x$with_tcl" = "xno"; then
  WHIPTCLSO=
else
  AC_CHECK_HEADER([tcl.h], [WHIPTCLSO=whiptcl.so], [WHIPTCLSO=])
fi
AC_SUBST([WHIPTCLSO])

AC_ARG_WITH([gpm-support], [  --with-gpm-support      compile with GPM support])

if test "x$with_gpm_support" = "xyes"; then
  AC_DEFINE([USE_GPM], 1, [Define to 1 if GPM support is enabled])
fi

AC_ARG_ENABLE([nls], [  --disable-nls           compile without NLS support])

if test "x$enable_nls" != "xno"; then
  AC_DEFINE([ENABLE_NLS], 1, [Define to 1 if NLS support is enabled])
fi

AC_CONFIG_FILES([Makefile libnewt.pc])
AC_OUTPUT

