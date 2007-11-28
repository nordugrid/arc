#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.56)
AC_CANNONICAL_SYSTEM
AC_INIT([arc],[0.9.0],[http://bugzilla.nordugrid.org/])
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
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

# Portable 64bit file offsets
AC_SYS_LARGEFILE

# Swig
AC_CHECK_PROGS(SWIG, swig)
AM_CONDITIONAL([SWIG_ENABLED],[test "x$SWIG" != "x"])

# Java
dnl Check if Java is explicitly disabled.
AC_ARG_ENABLE(java, AC_HELP_STRING([--disable-java], [disable the Java binding]),[],enable_java="yes")
AC_ARG_WITH(jdk, AC_HELP_STRING([--with-jdk=(JDK)], [path SUN JDK. If unset the system JDK will be used]))

JAVAC_FLAGS=
JDK_CFLAGS=
JDK_LIBS=

if test "X$with_jdk" != "X"; then

    # SUN JDK

    JAVA=$with_jdk/bin/java
    JAVAC=$with_jdk/bin/javac
    JAR=$with_jdk/bin/jar

    JDK_CFLAGS=-I$with_jdk/include
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
    JDK_CFLAGS="$JDK_CFLAGS $JDK_CFLAGS/$SUN_EXTRA_INCLUDE"

    SAVE_CPPFLAGS=$CPPFLAGS
    CPPFLAGS="$CPPFLAGS $JDK_CFLAGS"
    AC_CHECK_HEADERS(jni.h)
    CPPFLAGS=$SAVE_CPPFLAGS

    JNIH='<jni.h>'

    for jvm in $with_jdk/jre/lib/i386/server \
               $with_jdk/jre/lib/x86_64/server ; do
	if test -e $jvm/libjvm.so ; then
	    SAVE_LDFLAGS=$LDFLAGS
	    LDFLAGS="-L$jvm $LDFLAGS"
	    # delete cached test result...
	    unset ac_cv_lib_jvm_JNI_CreateJavaVM
	    AC_MSG_NOTICE([Checking for JVM in $jvm/libjvm.so])
	    AC_CHECK_LIB([jvm], [JNI_CreateJavaVM],
	                 [JDK_LIBS="-L$jvm -ljvm"], [])
	    LDFLAGS=$SAVE_LDFLAGS
	    test -n "$JDK_LIBS" && break
	fi
    done

else

    # System JDK

    AC_CHECK_HEADERS(jni.h JavaVM/jni.h)

    if test "X$ac_cv_header_JavaVM_jni_h" == "Xyes" ; then
	# MacOS X
	JNIH='<JavaVM/jni.h>'
	JDK_LIBS="-Wl,-framework -Wl,JavaVM"
    elif test "X$ac_cv_header_jni_h" == "Xyes" ; then
	# Linux
	JNIH='<jni.h>'
	# gcc 4.x
	for jvm in /usr/lib/gcj-`gcc -dumpversion` \
	           /usr/lib/gcj-`gcc -dumpversion|sed 's/\.[[0-9]]*$//'` \
	           /usr/lib64/gcj-`gcc -dumpversion` \
	           /usr/lib64/gcj-`gcc -dumpversion|sed 's/\.[[0-9]]*$//'` ; do
	    if test -e $jvm/libjvm.so ; then
		SAVE_LDFLAGS=$LDFLAGS
		LDFLAGS="-L$jvm $LDFLAGS"
		# delete cached test result...
		unset ac_cv_lib_jvm_JNI_CreateJavaVM
		AC_MSG_NOTICE([Checking for JVM in $jvm/libjvm.so])
		AC_CHECK_LIB([jvm], [JNI_CreateJavaVM],
		             [JDK_LIBS="-L$jvm -ljvm"], [])
		LDFLAGS=$SAVE_LDFLAGS
	    fi
	    test -n "$JDK_LIBS" && break
	done
	# gcc 3.x
	if test -z "$JDK_LIBS" ; then
	    AC_CHECK_LIB([gcj], [JNI_CreateJavaVM], [JDK_LIBS=-lgcj], [])
	fi
    fi
fi

AC_CHECK_PROGS(JAVA, java gij)
AC_CHECK_PROGS(JAVAC, javac gcj ecj)
AC_CHECK_PROGS(JAR, fastjar jar)

if test "X$JAVAC" != "X" && test "X`basename $JAVAC`" = "Xgcj"; then
    JAVAC_FLAGS=-C
fi

jni_h=no
if test "X$ac_cv_header_jni_h" == "Xyes" ||
   test "X$ac_cv_header_JavaVM_jni_h" == "Xyes" ; then
    jni_h=yes
fi

JAVAWRAP=no
if test -f java/arc_wrap.cpp || test "X$SWIG" != "X" ; then
    JAVAWRAP=yes
fi

if test "X$jni_h" = "Xyes" && test "X$JAVAWRAP" = "Xyes" &&
   test "X$JAVAC" != "X" && test "X$JAR" != "X" &&
   test "X$JDK_LIBS" != "X" ; then
    AC_MSG_NOTICE([Java available: $JAVAC])
else
    enable_java=no
fi

AC_MSG_NOTICE([Java enabled: $enable_java])

AM_CONDITIONAL([JAVA_ENABLED],[test "x$enable_java" = "xyes"])

AC_SUBST(JAVAC_FLAGS)
AC_SUBST(JDK_CFLAGS)
AC_SUBST(JDK_LIBS)
AC_SUBST(JNIH)

# Python
AC_ARG_ENABLE(python, AC_HELP_STRING([--disable-python], [disable the Python binding]),[],enable_python="yes")

AC_ARG_WITH(python, AC_HELP_STRING([--with-python=(PYTHON)], [set the full path to the python program to use]))

AC_CHECK_PROGS(PYTHON, $with_python python)

if test "X$PYTHON" != "X" && test "X$enable_python" = "Xyes"; then
    changequote(<<, >>)dnl
    PYTHON_VERSION=`$PYTHON -c 'import sys; print sys.version[:3]'`
    PYTHON_CFLAGS=-I`$PYTHON -c 'from distutils import sysconfig; print sysconfig.get_python_inc()'`
    PY_LIBS=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_config_vars().get('LIBS')" | sed s/None//`
    PY_SYSLIBS=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_config_vars().get('SYSLIBS')" | sed s/None//`
    PY_LIBDEST=`$PYTHON -c "from distutils import sysconfig; print sysconfig.get_config_vars().get('LIBDEST')" | sed s/None//`
    PYTHON_SITE_PACKAGES=`$PYTHON -c 'from distutils import sysconfig; print sysconfig.get_python_lib(1,0,"${prefix}")'`
    changequote([, ])dnl
    PYTHON_LIBS="$PY_LIBS $PY_SYSLIBS"
    SAVE_LDFLAGS=$LDFLAGS
    LDFLAGS="$PYTHON_LIBS $LDFLAGS"
    AC_CHECK_LIB([python$PYTHON_VERSION], [Py_Initialize],[
        AC_MSG_NOTICE([No additional path to python library needed])
        PYTHON_LIBS="-lpython$PYTHON_VERSION $PYTHON_LIBS"],[
        LDFLAGS="-L$PY_LIBDEST/config $LDFLAGS"
        # check a different symbol or else configure will used cached value
        AC_CHECK_LIB([python$PYTHON_VERSION], [Py_Finalize],[
            AC_MSG_NOTICE([Adding path to python library])
            PYTHON_LIBS="-L$PY_LIBDEST/config -lpython$PYTHON_VERSION $PYTHON_LIBS"],[
            AC_ERROR([Can't find python library])])])
    LDFLAGS=$SAVE_LDFLAGS
    AC_SUBST(PYTHON_VERSION)
    AC_SUBST(PYTHON_CFLAGS)
    AC_SUBST(PYTHON_LIBS)
    AC_SUBST(PYTHON_SITE_PACKAGES)
fi

PYTHONWRAP=no
if test -f python/arc_wrap.cpp || test "X$SWIG" != "X" ; then
    PYTHONWRAP=yes
fi

if test "X$PYTHONWRAP" = "Xyes" && test "X$PYTHON" != "X"; then
    AC_MSG_NOTICE([Python available: $PYTHON_VERSION])
else
    enable_python=no
fi

AC_MSG_NOTICE([Python enabled: $enable_python])

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
SAVE_CPPFLAGS=$CPPFLAGS
AC_LANG_SAVE
AC_LANG_CPLUSPLUS
CPPFLAGS="$CPPFLAGS $GLIBMM_CFLAGS"
AC_CHECK_HEADER(glibmm/optiongroup.h,[glibmm_optiongroup_h=yes],[glibmm_optiongroup_h=no])
if test "$glibmm_optiongroup_h" = yes; then
  AC_DEFINE(HAVE_GLIBMM_OPTIONS,,[define if glibmm have support for command line arguments])
  AC_TRY_COMPILE([#include <glibmm.h>],[Glib::OptionEntry::Flags flags = Glib::OptionEntry::FLAG_NO_ARG;],[glibmm_optionflags=yes],[glibmm_optionflags=no])
  if test "$glibmm_optionflags" = yes; then
    AC_DEFINE(HAVE_GLIBMM_OPTIONFLAGS,,[define if glibmm have proper support for option flags])
  else  
    AC_MSG_NOTICE([WARNING: glibmm has no all arguments flags defined - arguments parsing may be inconsistent])
  fi
else
  AC_MSG_NOTICE([WARNING: glibmm has no support for command lineparsing - using reduced functionality])  
fi
AC_TRY_COMPILE([#include <glibmm.h>],[Glib::SignalChildWatch watch = Glib::signal_child_watch();],[glibmm_childwatch=yes],[glibmm_childwatch=no])
if test "$glibmm_childwatch" = yes; then
  AC_DEFINE(HAVE_GLIBMM_CHILDWATCH,,[define if glibmm have support for controling state of children processes])
else  
  AC_MSG_NOTICE([WARNING: glibmm has no API for controlling children processes - result of external processes may be inconsistent])
fi
AC_LANG_RESTORE
CPPFLAGS=$SAVE_CPPFLAGS

# check libxml
PKG_CHECK_MODULES(LIBXML2, [libxml-2.0 >= 2.4.0])
AC_SUBST(LIBXML2_CFLAGS)
AC_SUBST(LIBXML2_LIBS)

# check openssl
PKG_CHECK_MODULES(OPENSSL, [openssl >= 0.9.7a])
AC_SUBST(OPENSSL_CFLAGS)
AC_SUBST(OPENSSL_LIBS)
OPENSSL_PROXY=no
"$PKG_CONFIG" --exists 'openssl >= 0.9.7g'
if test "$?" = '0' ; then
	OPENSSL_PROXY=yes
        AC_MSG_NOTICE([Found OpenSSL with PROXY support])
else
        AC_MSG_NOTICE([WARNING: OpenSSL does not contain PROXY support])
fi
OPENSSL_X509_VERIFY_PARAM=no
"$PKG_CONFIG" --exists 'openssl >= 0.9.8'
if test "$?" = '0' ; then
	OPENSSL_X509_VERIFY_PARAM=yes
        AC_MSG_NOTICE([Found OpenSSL with X509_VERIFY_PARAM])
else
        AC_MSG_NOTICE([WARNING: OpenSSL does not contain X509_VERIFY_PARAM])
fi
OPENSSL_OLDRSA=yes
"$PKG_CONFIG" --exists 'openssl >= 0.9.8a'
if test "$?" = '0' ; then
	OPENSSL_OLDRSA=no
fi

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

# Check for explicitely disabled packages
# A-Rex
AC_ARG_ENABLE(a_rex_service, AC_HELP_STRING([--disable-a-rex-service], [disable building A-Rex service]),[],enable_a_rex_service="yes")
AC_MSG_NOTICE([A-Rex service enabled: $enable_a_rex_service])
AM_CONDITIONAL([A_REX_SERVICE_ENABLED],[test "x$enable_a_rex_service" = "xyes"])

# Setup conditionals
AM_CONDITIONAL([RLS_ENABLED], test -n "$GLOBUS_RLS_CLIENT_VERSION")
AM_CONDITIONAL([GRIDFTP_ENABLED], test -n "$GLOBUS_FTP_CLIENT_VERSION")
AM_CONDITIONAL([LFC_ENABLED], test x$LFC = xyes)
AM_CONDITIONAL([RSL_ENABLED], test -n "$GLOBUS_RSL_VERSION")
AM_CONDITIONAL([GSOAP_ENABLED], test x$GSOAP = xyes)
AM_CONDITIONAL([OPENSSL_PROXY_ENABLED], test x$OPENSSL_PROXY = xyes)
AM_CONDITIONAL([OPENSSL_X509_VERIFY_PARAM_ENABLED], test x$OPENSSL_X509_VERIFY_PARAM = xyes)

# Setup defines
if test -n "$GLOBUS_RSL_VERSION"; then
    AC_DEFINE(HAVE_GLOBUS_RSL,,[define if GLOBUS RSL package is available])
fi
if test -n "$GLOBUS_RLS_CLIENT_VERSION"; then
    AC_DEFINE(HAVE_GLOBUS_RLS_CLIENT,,[define if GLOBUS RSL package is available])
fi
if test x"$GSOAP" = xyes; then
    AC_DEFINE(HAVE_GSOAP,,[define if GSOAP is available])
fi
if test x"$OPENSSL_PROXY" = xyes; then
    AC_DEFINE(HAVE_OPENSSL_PROXY,,[define if OPENSSL has PROXY capabilities])
fi
if test x"$OPENSSL_X509_VERIFY_PARAM" = xyes; then
    AC_DEFINE(HAVE_OPENSSL_X509_VERIFY_PARAM,,[define if OPENSSL has X509_VERIFY_PARAM structure])
fi
if test x"$OPENSSL_OLDRSA" = xyes; then
    AC_DEFINE(HAVE_OPENSSL_OLDRSA,,[define if OPENSSL has old RSA generation interface])
fi

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
AC_CHECK_MEMBERS([struct stat.st_blksize])
AC_HEADER_TIME
AC_STRUCT_TM
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
AC_FUNC_STRERROR_R
AC_FUNC_STAT
AC_CHECK_FUNCS([acl bzero dup2 floor ftruncate gethostbyname_r gethostname getpid gmtime_r lchown localtime_r memchr memmove memset mkdir mkfifo regcomp rmdir select sigwaitinfo setenv socket strcasecmp strchr strcspn strdup strerror strncasecmp strstr strtol strtoul strtoull timegm tzset unsetenv])
AC_CHECK_LIB([resolv], [res_query], LIBRESOLV=-lresolv, LIBRESOLV=)
AC_CHECK_LIB([resolv], [__dn_skipname], LIBRESOLV=-lresolv, )
AC_SUBST(LIBRESOLV)


# check for platfom specific extra libraries

EXTRA_LIBS=""
REGEX_LIBS=""
SOCKET_LIBS=""

case "${host}" in
  *-*-mingw32)
    REGEX_LIBS="-lgnurx"
    SOCKET_LIBS="-lws2_32"
    EXTRA_LIBS="-lole32" 
  ;;
esac
AC_SUBST(EXTRA_LIBS)
AC_SUBST(REGEX_LIBS)
AC_SUBST(SOCKET_LIBS)

case " $LDFLAGS " in
     " -no-undefined ") ;;
     *) LDFLAGS="$LDFLAGS -no-undefined" ;;
esac

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/hed/Makefile
                 src/hed/libs/common/Makefile
                 src/hed/libs/common/counter/Makefile
                 src/hed/libs/data/Makefile
                 src/hed/libs/Makefile
                 src/hed/libs/loader/Makefile
                 src/hed/libs/message/Makefile
                 src/hed/libs/security/Makefile
                 src/hed/libs/security/ArcPDP/Makefile
                 src/hed/libs/security/ArcPDP/attr/Makefile
                 src/hed/libs/security/ArcPDP/policy/Makefile
                 src/hed/libs/security/ArcPDP/alg/Makefile
                 src/hed/libs/security/ArcPDP/fn/Makefile
                 src/hed/libs/wsrf/Makefile
                 src/hed/libs/ws-addressing/Makefile
                 src/hed/libs/infosys/Makefile
                 src/hed/libs/delegation/Makefile
                 src/hed/libs/ws/Makefile
                 src/hed/libs/misc/Makefile
                 src/hed/daemon/Makefile
                 src/hed/daemon/scripts/Makefile
                 src/hed/mcc/Makefile
                 src/hed/mcc/soap/Makefile
                 src/hed/mcc/tcp/Makefile
                 src/hed/mcc/http/Makefile
                 src/hed/mcc/tls/Makefile
                 src/hed/dmc/Makefile
                 src/hed/dmc/file/Makefile
                 src/hed/dmc/gridftp/Makefile
                 src/hed/dmc/http/Makefile
                 src/hed/dmc/lfc/Makefile
                 src/hed/dmc/rls/Makefile
                 src/hed/pdc/Makefile
                 src/services/Makefile
                 src/services/echo_java/Makefile
                 src/services/echo_python/Makefile
                 src/services/a-rex/Makefile
                 src/services/a-rex/ldif/Makefile
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
                 src/services/a-rex/config/Makefile
                 src/services/pdp/Makefile
                 src/services/httpd/Makefile
                 src/services/arex2/Makefile
                 src/services/isis/Makefile
                 src/clients/Makefile
                 src/clients/a-rex/Makefile
                 src/clients/data/Makefile
                 src/clients/echo/Makefile
                 src/clients/isis/Makefile
                 src/clients/wsrf/Makefile
                 src/tests/Makefile
                 src/tests/data/Makefile
                 src/tests/echo/Makefile
                 src/tests/xpath/Makefile
                 src/utils/Makefile
                 swig/Makefile
                 java/Makefile
                 python/Makefile
                 intl/Makefile
                 doc/Makefile
                 include/Makefile
                 debian/Makefile 
                 arc.spec
                 src/hed/daemon/arched.8
])
AC_OUTPUT

AC_MSG_RESULT([
Java binding:       ${enable_java}
Python binding:     ${enable_python}

Available third-party features:

RLS:                ${enable_rls}
GridFTP:            ${enable_gridftp}
LFC:                ${enable_lfc}
RSL:                ${enable_rsl}
gSOAP:              ${enable_gsoap}

Included components:
A-Rex service:      ${enable_a_rex_service}
])
