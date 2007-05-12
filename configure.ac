#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_CANNONICAL_SYSTEM
AC_INIT([arc],[1.0],[knowarc-support@nordugrid.org])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC_STDC
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AM_PROG_LIBTOOL

# Checks for language bindings 
dnl Detect available languages binding.
languages_available=

AC_CHECK_PROGS(SWIG, swig)
# Java
AC_CHECK_PROGS(JAVA, java)
AC_CHECK_PROGS(JAVAC, gcj javac)
AC_CHECK_PROGS(JAR, fastjar jar)
AC_ARG_ENABLE(java, [  --disable-java   disable Java binding],,enable_java="yes")
AC_ARG_WITH(java-home, [  --with-java-home=(JAVA_HOME) set full path to the java home directory.])
JAVAC_FLAGS=
JDK_INCLUDE=
SUN=no

if test "x$with_java_home" != "x"; then
    JDK_INCLUDE="$with_java_home/include"
fi

if test "x$JAVA" = "xjava" && test "x$JAVAC" = "xjavac"; then
    dnl SUN Java
    changequote(<<, >>)dnl
    JAVA_VERSION=`$JAVA -version 2>&1 | $SED -ne 's/java version "\([^"]*\)".*/\1/p' 2>/dev/null`
    changequote([, ])dnl
    dnl eg. Kaffe env
    if test "x$JAVA_VERSION" = "x"; then
        JAVA_VERSION=`$JAVA -version 2>&1 | grep "Java Version" | $SED 's/^.*Java Version: //g'`
        if test "x$JAVA_VERSION" != "x"; then
            JDK_INCLUDE="/usr/lib/kaffe/include"
        fi
    else
        dnl We assume SUN env first
        SUN=yes
    fi
    dnl jni.h is required
    if ! test "$JDK_INCLUDE/jni.h"; then
        JAVA_VERSION=""
    fi
fi

dnl transform JDK_INCLUDE in CFLAGS
SUN_EXTRA_INCLUDE=
case "${host}" in
    *-pc-mingw32)
        SUN_EXTRA_INCLUDE="win32" 
        ;;
    *-pc-cygwin)
        SUN_EXTRA_INCLUDE="win32" 
        ;;
    *linux*)
        SUN_EXTRA_INCLUDE="linux"
        ;;
esac

if test "x$JDK_INCLUDE" != "x"; then
    if test "x$SUN" = "xyes"; then
        JDK_INCLUDE="$JDK_INCLUDE -I$JDK_INCLUDE/$SUN_EXTRA_INCLUDE"
    fi
    JDK_INCLUDE=-I$JDK_INCLUDE
fi

if test "X$JAVA_VERSION" != "X" && test "X$JAR" != "X"; then
    languages_available="$languages_available $JAVA($JAVA_VERSION)"
else
    enable_java=no
    JAVA_VERSION=""
fi

dnl Conditional java sub dir test.
AM_CONDITIONAL([JAVA_ENABLED],[test "x$enable_java" = "xyes"])
AC_SUBST(JAVA_VERSION)
AC_SUBST(JAVAC_FLAGS)
AC_SUBST(JDK_INCLUDE)

# Python
AC_CHECK_PROGS(PYTHON, python)

dnl Check if python is explicitly disabled.
AC_ARG_ENABLE(python, [  --disable-python        disable the Python binding],,
              enable_python="yes")

dnl Check if user passed a specific python program.
AC_ARG_WITH(python,
            [  --with-python=(PYTHON)  set the full path to the python program t
o use.])

dnl specific program passed, set PYTHON to it.
if test "X$with_python" != "X"; then
    PYTHON=$with_python
fi

dnl need to change quotes to allow square brackets
changequote(<<, >>)dnl
PYTHON_VERSION=`$PYTHON -c 'import sys; print sys.version[:3]' 2>/dev/null`
changequote([, ])dnl

test "x$PYTHON" != "x" || AC_MSG_ERROR(Python must be installed to compile lasso
)

AC_MSG_CHECKING(for Python development files)
dnl Check if we were be able to extract a good version number.
if test "X$PYTHON_VERSION" != "X"; then
    PY_PREFIX=`$PYTHON -c 'import sys ; print sys.prefix'`
    PY_EXEC_PREFIX=`$PYTHON -c 'import sys ; print sys.exec_prefix'`
    changequote(<<, >>)dnl
    PY_VERSION=`$PYTHON -c 'import sys ; print sys.version[0:3]'`
    changequote([, ])dnl
    PYTHON_H=$PY_PREFIX/include/python$PY_VERSION/Python.h
    LIBPYTHON_SO=$PY_PREFIX/lib/libpython$PY_VERSION.so
    if test -f $PYTHON_H; then
            dnl Not useful unless we make a test on the python version.
            PYTHON="python$PY_VERSION"
            PY_CFLAGS="-I$PY_PREFIX/include/python$PY_VERSION"
            PY_MAKEFILE="$PY_EXEC_PREFIX/lib/python$PY_VERSION/config/Makefile"
            PY_OTHER_LIBS=`$SED -n -e 's/^LIBS=\(.*\)/\1/p' $PY_MAKEFILE`
            PY_EXTRA_LIBS="$PY_LOCALMODLIBS $PY_BASEMODLIBS $PY_OTHER_LIBS"
            PY_SITE_PACKAGES="\${prefix}/lib/python$PY_VERSION/site-packages"
            AC_SUBST(PYTHON)
            AC_SUBST(PY_LIB_LOC)
            AC_SUBST(PY_CFLAGS)
            AC_SUBST(PY_EXTRA_LIBS)
            AC_SUBST(PY_DYNLOAD)
            AC_SUBST(PY_LIB_A)
            AC_SUBST(PY_SITE_PACKAGES)
            languages_available="$languages_available python($PYTHON_VERSION)"
    else
            enable_python=no
    fi
else
        enable_python=no
fi
AM_CONDITIONAL([PYTHON_ENABLED],[test "x$enable_python" = "xyes"])
AC_MSG_RESULT($enable_python)
AC_SUBST(PYTHON_VERSION)

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "xPKG_CONFIG" = "xno"; then
        AC_MSG_ERROR([ *** pkg-config not found])
else
        pkgconfigdir=${libdir}/pkgconfig
        AC_SUBST(pkgconfigdir)
fi


# using pkgconfig
# check glibmm
PKG_CHECK_MODULES(GLIBMM, [glibmm-2.4 >= 2.6.0])
AC_SUBST(GLIBMM_CFLAGS)
AC_SUBST(GLIBMM_LIBS)
PKG_CHECK_MODULES(LIBXML2, [libxml-2.0 >= 2.6.0])
AC_SUBST(LIBXML2_CFLAGS)
AC_SUBST(LIBXML2_LIBS)

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h netdb.h stdlib.h sys/socket.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_OFF_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_REALLOC
AC_CHECK_FUNCS([memmove memset socket strcasecmp strchr strncasecmp strtol])

AC_OUTPUT([Makefile \
                 src/Makefile \
                 src/hed/Makefile \
                 src/hed/wsrf/Makefile \
                 src/hed/ws-addressing/Makefile \
                 src/hed/libs/Makefile \
                 src/hed/libs/loader/Makefile \
                 src/hed/libs/message/Makefile \
		         src/hed/mcc/Makefile \
		         src/hed/mcc/soap/Makefile \
		         src/hed/mcc/tcp/Makefile \
		         src/hed/mcc/http/Makefile \
                 src/libs/Makefile \
                 src/libs/common/Makefile \
                 src/tests/Makefile \
                 src/tests/echo/Makefile \
                 swig/Makefile \
                 java/Makefile \
                 python/Makefile \
		         doc/Makefile
])

languages_available=`echo $languages_available | sed -e "s/^ //" `
AC_MSG_RESULT(
Available languages: ${languages_available}

Java binding:       ${enable_java}
Python binding      ${enable_python}
)
