dnl @synopsis AC_CHECK_CLASSPATH
dnl
dnl AC_CHECK_CLASSPATH just displays the CLASSPATH, for the edification
dnl of the user.
dnl
dnl Note: This is part of the set of autoconf M4 macros for Java programs.
dnl It is VERY IMPORTANT that you download the whole set, some
dnl macros depend on other. Unfortunately, the autoconf archive does not
dnl support the concept of set of macros, so I had to break it for
dnl submission.
dnl The general documentation, as well as the sample configure.in, is
dnl included in the AC_PROG_JAVA macro.
dnl
dnl @author Stephane Bortzmeyer <bortzmeyer@pasteur.fr>
dnl @version $Id: ac_check_classpath.m4,v 12.0 2004/11/17 03:43:38 bostic Exp $
dnl
AC_DEFUN([AC_CHECK_CLASSPATH],[
if test "x$CLASSPATH" = x; then
        echo "You have no CLASSPATH, I hope it is good"
else
        echo "You have CLASSPATH $CLASSPATH, hope it is correct"
fi
])
