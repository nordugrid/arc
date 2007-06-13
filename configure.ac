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

# Swig
AC_CHECK_PROGS(SWIG, swig)

# Java
dnl Check if Java is explicitly disabled.
AC_ARG_ENABLE(java, AC_HELP_STRING([--disable-java], [disable the Java binding]),[],enable_java="yes")
AC_ARG_WITH(jdk, AC_HELP_STRING([--with-jdk=(JDK)], [set the full path JDK include]))

AC_CHECK_PROGS(JAVA, java gij)
AC_CHECK_PROGS(JAVAC, javac gcj ecj)
AC_CHECK_PROGS(JAR, fastjar jar)

JAVAC_FLAGS=
if test "X`basename $JAVAC`" = "Xgcj"; then
    JAVAC_FLAGS=-C
fi

JDK_INCLUDE=$with_jdk
# XXX: temporaly solution
JDK_LDFLAGS="-L$with_jdk/jre/lib/i386/server/ -ljvm"
if test "X$JAVAC" != "X"; then
    if test "X$JDK_INCLUDE" == "X"; then
        JDK_INCLUDE=`which $JAVAC`
        JDK_INCLUDE=`dirname $JDK_INCLUDE`
        JDK_INCLUDE=`dirname $JDK_INCLUDE`
        # XXX: temporaly solution
        JDK_LDFLAGS="-L$JDK_INCLUDE/jre/lib/i386/server/ -ljvm"
    fi

    if test "x$JDK_INCLUDE" = "x/" || test "x$JDK_INCLUDE" = "x/usr"; then
        JDK_INCLUDE=
    else
        JDK_INCLUDE=-I$JDK_INCLUDE/include
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
        JDK_INCLUDE="$JDK_INCLUDE $JDK_INCLUDE/$SUN_EXTRA_INCLUDE"
    fi
fi

SAVE_CPPFLAGS=$CPPFLAGS
CPPFLAGS=$JDK_INCLUDE
SAVA_CFLAGS=$CFLAGS
CFLAGS=$JDK_INCLUDE
AC_CHECK_HEADER(jni.h,[jni_h=yes],[jni_h=no])
CPPFLAGS=$SAVE_CPPFLAGS
CFLAGS=$SAVA_CFLAGS

if test "x$jni_h" = "xyes" && test "X$SWIG" != "X" &&
   test "X$JAVAC" != "X" && test "X$JAR" != "X"; then
    languages_available="$languages_available Java($JAVAC)"
else
    enable_java=no
fi

dnl Conditional java sub dir test.
AM_CONDITIONAL([JAVA_ENABLED],[test "x$enable_java" = "xyes"])
AC_SUBST(JAVAC_FLAGS)
AC_SUBST(JDK_INCLUDE)
AC_SUBST(JDK_LDFLAGS)

# Python
AC_ARG_ENABLE(python, AC_HELP_STRING([--disable-python], [disable the Python binding]),[],enable_python="yes")

AC_ARG_WITH(python, AC_HELP_STRING([--with-python=(PYTHON)], [set the full path to the python program to use]))

AC_CHECK_PROGS(PYTHON, $with_python python)

if test "X$PYTHON" != "X"; then
    changequote(<<, >>)dnl
    PYTHON_VERSION=`$PYTHON -c 'import sys; print sys.version[:3]'`
    changequote([, ])dnl
    PY_CFLAGS=-I`$PYTHON -c 'from distutils import sysconfig; print sysconfig.get_python_inc()'`
    changequote(<<, >>)dnl
    PY_EXTRA_LIBS=`$PYTHON -c "from distutils import sysconfig; print sysconfig.parse_makefile(sysconfig.get_makefile_filename())['LIBS']"`
    changequote([, ])dnl
    PY_SITE_PACKAGES=`$PYTHON -c 'from distutils import sysconfig; print sysconfig.get_python_lib(1,0,"${prefix}")'`
    AC_SUBST(PYTHON_VERSION)
    AC_SUBST(PY_CFLAGS)
    AC_SUBST(PY_EXTRA_LIBS)
    AC_SUBST(PY_SITE_PACKAGES)
fi

if test "X$SWIG" != "X" && test "X$PYTHON" != "X"; then
    languages_available="$languages_available Python($PYTHON_VERSION)"
else
    enable_python=no
fi

AM_CONDITIONAL([PYTHON_ENABLED],[test "x$enable_python" = "xyes"])

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "xPKG_CONFIG" = "xno"; then
        AC_MSG_ERROR([ *** pkg-config not found])
else
        pkgconfigdir=${libdir}/pkgconfig
        AC_SUBST(pkgconfigdir)
fi


# using pkgconfig
# check gthread
PKG_CHECK_MODULES(GTHREAD, [gthread-2.0 >= 2.4.7])
AC_SUBST(GTHREAD_CFLAGS)
AC_SUBST(GTHREAD_LIBS)
# check glibmm
PKG_CHECK_MODULES(GLIBMM, [glibmm-2.4 >= 2.4.7])
AC_SUBST(GLIBMM_CFLAGS)
AC_SUBST(GLIBMM_LIBS)
# check libxml
PKG_CHECK_MODULES(LIBXML2, [libxml-2.0 >= 2.6.0])
AC_SUBST(LIBXML2_CFLAGS)
AC_SUBST(LIBXML2_LIBS)
# check openssl
PKG_CHECK_MODULES(OPENSSL, [openssl >= 0.9.7a])
AC_SUBST(OPENSSL_CFLAGS)
AC_SUBST(OPENSSL_LIBS)

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
                 src/libs/Makefile \
                 src/libs/common/Makefile \
                 src/hed/Makefile \
                 src/hed/wsrf/Makefile \
                 src/hed/ws-addressing/Makefile \
                 src/hed/libs/Makefile \
                 src/hed/libs/loader/Makefile \
                 src/hed/libs/message/Makefile \
                 src/hed/libs/counter/Makefile \
                 src/hed/mcc/Makefile \
                 src/hed/mcc/soap/Makefile \
                 src/hed/mcc/tcp/Makefile \
                 src/hed/mcc/http/Makefile \
                 src/hed/mcc/tls/Makefile \
                 src/hed/mcc/plexer/Makefile \
                 src/services/Makefile \
                 src/services/echo_java/Makefile \
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
