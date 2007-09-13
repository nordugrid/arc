#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.57)
AC_CANNONICAL_SYSTEM
AC_INIT([arc],[1.0],[knowarc-support@nordugrid.org])
#AM_INIT_AUTOMAKE([-Wno-portability])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC_STDC
AC_PROG_CPP
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
# AC_PROG_RANLIB
AM_PROG_LIBTOOL

# Checks for language bindings 
dnl Detect available languages binding.
languages_available=

# Swig
AC_CHECK_PROGS(SWIG, swig)
AM_CONDITIONAL([SWIG_ENABLED],[test "x$SWIG" != "x"])

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
#JDK_LDFLAGS="-L$with_jdk/jre/lib/i386/server/ -ljvm"
if test "X$JAVAC" != "X"; then
    if test "X$JDK_INCLUDE" == "X"; then
        JDK_INCLUDE=`which $JAVAC`
        JDK_INCLUDE=`dirname $JDK_INCLUDE`
        JDK_INCLUDE=`dirname $JDK_INCLUDE`
        # XXX: temporaly solution
#        JDK_LDFLAGS="-L$JDK_INCLUDE/jre/lib/i386/server/ -ljvm"
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
CPPFLAGS="$CPPFLAGS $JDK_INCLUDE"
AC_CHECK_HEADER(jni.h,[jni_h=yes],[jni_h=no])
CPPFLAGS=$SAVE_CPPFLAGS

if test "x$jni_h" = "xyes" && test "X$SWIG" != "X" &&
   test "X$JAVAC" != "X" && test "X$JAR" != "X"; then
    languages_available="$languages_available Java($JAVAC)"
else
    enable_java=no
fi

AC_MSG_NOTICE([Java enabled: $enable_java])

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
    PY_EXTRA_LIBS="-lpython$PYTHON_VERSION $PY_EXTRA_LIBS"
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
if test "x$PKG_CONFIG" = "xno"; then
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
SAVE_CXXFLAGS=$CXXFLAGS
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
CXXFLAGS="$CXXFLAGS $GLIBMM_CFLAGS"
AC_TRY_COMPILE([#include <glibmm/optiongroup.h>],[],[glibmm_optiongroup_h=yes],[glibmm_optiongroup_h=no])
AC_LANG_RESTORE
CXXFLAGS=$SAVE_CXXFLAGS
if test "$glibmm_optiongroup_h" = yes; then
  AC_DEFINE(HAVE_GLIBMM_OPTIONS,,[define if glibmm have support for command line arguments])
fi
# check libxml
PKG_CHECK_MODULES(LIBXML2, [libxml-2.0 >= 2.6.0])
AC_SUBST(LIBXML2_CFLAGS)
AC_SUBST(LIBXML2_LIBS)
# check openssl
PKG_CHECK_MODULES(OPENSSL, [openssl >= 0.9.7a])
AC_SUBST(OPENSSL_CFLAGS)
AC_SUBST(OPENSSL_LIBS)
# check gSOAP
PKG_CHECK_MODULES(GSOAP, [gsoap++ >= 2.7], [GSOAP=yes], [GSOAP=no])
AC_SUBST(GSOAP_CFLAGS)
AC_SUBST(GSOAP_LIBS)
WSDL2H=wsdl2h
SOAPCPP2=soapcpp2
AC_SUBST(WSDL2H)
AC_SUBST(SOAPCPP2)

if test "x$GSOAP" = "xyes"; then
	# gSOAP version
	GSOAP_VERSION=`$SOAPCPP2 -v 2>&1 | grep 'Compiler' | sed 's/^.*\([[0-9]]\.[[0-9]]\.[[0-9]][[a-z]]\).*$/\1/'`
        AC_MSG_NOTICE([Found gsoap version: $GSOAP_VERSION])
	GSOAP_VERSION_MAJOR=`echo $GSOAP_VERSION | sed 's/^\([[0-9]]\).*/\1/'`
	GSOAP_VERSION_MINOR=`echo $GSOAP_VERSION | sed 's/^[[0-9]]\.\([[0-9]]\).*/\1/'`
	GSOAP_VERSION_SUBMINOR=`echo $GSOAP_VERSION | sed 's/^[[0-9]]\.[[0-9]]\.\([[0-9]]\).*/\1/'`
	gsoap_279=no
	if test "$GSOAP_VERSION_MAJOR" -gt '2'; then
	  gsoap_279=yes
	elif test "$GSOAP_VERSION_MAJOR" = '2'; then
	  if test "$GSOAP_VERSION_MINOR" -gt '7'; then
	    gsoap_279=yes
	  elif test "$GSOAP_VERSION_MINOR" = '7'; then
	    if test "$GSOAP_VERSION_SUBMINOR" -ge '9'; then
	      gsoap_279=yes
	    fi
	  fi
	fi
	if test "x$gsoap_279" = "xyes"; then
	  AC_DEFINE(GSOAP_ALWAYS_USES_NAMESPACES_FOR_MEMBER_NAMES,,[define if gSOAP prepends names of class members with namespace])
	fi
fi

# globus/gpt packages
GPT_PKG(globus_rls_client)
AC_SUBST(GLOBUS_RLS_CLIENT_CFLAGS)
AC_SUBST(GLOBUS_RLS_CLIENT_LIBS)
GPT_PKG(globus_ftp_client)
AC_SUBST(GLOBUS_FTP_CLIENT_CFLAGS)
AC_SUBST(GLOBUS_FTP_CLIENT_LIBS)
GPT_PKG(globus_rsl)
AC_SUBST(GLOBUS_RSL_CFLAGS)
AC_SUBST(GLOBUS_RSL_LIBS)

# Check for gridftp-v2 
SAVE_CFLAGS=$CFLAGS
SAVE_LIBS=$LIBS
CFLAGS="$CFLAGS $GLOBUS_FTP_CLIENT_CFLAGS"
LIBS="$LIBS $GLOBUS_FTP_CLIENT_LIBS"
AC_CHECK_FUNCS(globus_ftp_client_handleattr_set_gridftp2)
CFLAGS=$SAVE_CFLAGS
LIBS=$SAVE_LIBS

# Check for LFC
AC_CHECK_HEADER(lfc/lfc_api.h,[LFC=yes],[LFC=no])

# Setup conditionals
AM_CONDITIONAL([RLS_ENABLED], test -n "$GLOBUS_RLS_CLIENT_VERSION")
AM_CONDITIONAL([GRIDFTP_ENABLED], test -n "$GLOBUS_FTP_CLIENT_VERSION")
AM_CONDITIONAL([LFC_ENABLED], test x$LFC = xyes)
AM_CONDITIONAL([RSL_ENABLED], test -n "$GLOBUS_RSL_VERSION")
AM_CONDITIONAL([GSOAP_ENABLED], test x$GSOAP = xyes)

# Setup messages for reporting
enable_rls=no
if test -n "$GLOBUS_RLS_CLIENT_VERSION" ; then enable_rls=yes; fi
enable_gridftp=no
if test -n "$GLOBUS_FTP_CLIENT_VERSION" ; then enable_gridftp=yes; fi
enable_lfc=no
if test x$LFC = xyes ; then enable_lfc=yes; fi
enable_rsl=no
if test -n "$GLOBUS_RSL_VERSION" ; then enable_rsl=yes; fi
enable_gsoap=no
if test x$GSOAP = xyes ; then enable_gsoap=yes; fi

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
# AC_HEADER_RESOLV
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h libintl.h limits.h netdb.h netinet/in.h stdint.h stdlib.h string.h sys/file.h sys/socket.h sys/vfs.h unistd.h])
AC_CXX_HAVE_SSTREAM

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_UID_T
AC_C_INLINE
AC_TYPE_MODE_T
AC_TYPE_OFF_T
AC_TYPE_PID_T
AC_TYPE_SIZE_T
#AC_TYPE_SSIZE_T
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_HEADER_TIME
AC_STRUCT_TM
#AC_TYPE_UINT32_T
#AC_TYPE_UINT64_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_FUNC_CHOWN
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_LSTAT
AC_FUNC_LSTAT_FOLLOWS_SLASHED_SYMLINK
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_FUNC_SELECT_ARGTYPES
AC_FUNC_SETPGRP
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_CHECK_FUNCS([acl bzero dup2 floor ftruncate gethostbyname_r gethostname gmtime_r lchown localtime_r memchr memmove memset mkdir mkfifo regcomp rmdir select sigwaitinfo setenv socket strcasecmp strchr strcspn strdup strerror strncasecmp strstr strtol strtoul strtoull tzset unsetenv])
AC_CHECK_LIB([resolv], [res_query], LIBRESOLV=-lresolv, LIBRESOLV=)
AC_CHECK_LIB([resolv], [__dn_skipname], LIBRESOLV=-lresolv, )
AC_SUBST(LIBRESOLV)


AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/hed/Makefile
                 src/hed/libs/common/Makefile
                 src/hed/libs/data/Makefile
                 src/hed/libs/Makefile
                 src/hed/libs/loader/Makefile
                 src/hed/libs/message/Makefile
                 src/hed/libs/counter/Makefile
                 src/hed/libs/security/Makefile
                 src/hed/libs/security/ArcPDP/Makefile
                 src/hed/libs/security/ArcPDP/attr/Makefile
                 src/hed/libs/security/ArcPDP/policy/Makefile
                 src/hed/libs/security/ArcPDP/alg/Makefile
                 src/hed/libs/security/ArcPDP/fn/Makefile
                 src/hed/libs/wsrf/Makefile
                 src/hed/libs/ws-addressing/Makefile
                 src/hed/libs/infosys/Makefile
                 src/hed/daemon/Makefile
                 src/hed/mcc/Makefile
                 src/hed/mcc/soap/Makefile
                 src/hed/mcc/tcp/Makefile
                 src/hed/mcc/http/Makefile
                 src/hed/mcc/tls/Makefile
                 src/hed/dmc/Makefile
                 src/hed/dmc/gridftp/Makefile
                 src/hed/dmc/http/Makefile
                 src/hed/dmc/lfc/Makefile
                 src/hed/dmc/rls/Makefile
                 src/services/Makefile
                 src/services/echo_java/Makefile
                 src/services/echo_python/Makefile
                 src/services/a-rex/Makefile
                 src/services/a-rex/grid-manager/Makefile
                 src/services/a-rex/grid-manager/cache/Makefile
                 src/services/a-rex/grid-manager/conf/Makefile
                 src/services/a-rex/grid-manager/files/Makefile
                 src/services/a-rex/grid-manager/jobdesc/Makefile
                 src/services/a-rex/grid-manager/jobdesc/jsdl/Makefile
                 src/services/a-rex/grid-manager/jobdesc/rsl/Makefile
                 src/services/a-rex/grid-manager/jobs/Makefile
                 src/services/a-rex/grid-manager/log/Makefile
                 src/services/a-rex/grid-manager/mail/Makefile
                 src/services/a-rex/grid-manager/misc/Makefile
                 src/services/a-rex/grid-manager/run/Makefile
                 src/services/a-rex/grid-manager/url/Makefile
                 src/clients/Makefile
                 src/clients/a-rex/Makefile
                 src/tests/Makefile
                 src/tests/data/Makefile
                 src/tests/echo/Makefile
                 swig/Makefile
                 java/Makefile
                 python/Makefile
                 intl/Makefile
                 doc/Makefile
                 include/Makefile
                 arc.spec
                 src/hed/daemon/arcserver.8
])
AC_OUTPUT

languages_available=`echo $languages_available | sed -e "s/^ //" `
AC_MSG_RESULT([
Available languages: ${languages_available}

Java binding:       ${enable_java}
Python binding:     ${enable_python}

Available third-party features:

RLS:                ${enable_rls}
GridFTP:            ${enable_gridftp}
LFC:                ${enable_lfc}
RSL:                ${enable_rsl}
gSOAP:              ${enable_gsoap}

])
