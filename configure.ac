#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.56)
AC_CANNONICAL_SYSTEM
AC_INIT([arc],[0.9.0],[http://bugzilla.nordugrid.org/])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])

m4_pattern_allow([AC_PATH_PROG])
m4_pattern_allow([AC_MSG_WARN])

AC_PROG_CXX
AC_PROG_CC_STDC
AC_PROG_CPP
AC_GNU_SOURCE
AC_PROG_AWK
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_DISABLE_STATIC
AC_LIBTOOL_WIN32_DLL
AM_PROG_LIBTOOL

# gettext
gettext_available="yes"
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.11.3])
[[ -e po/POTFILES.in ]] || touch po/POTFILES.in
AM_CONDITIONAL([GETTEXT_ENABLED],[test "x$gettext_available" = "xyes"])

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
	*solaris*)
	    SUN_EXTRA_INCLUDE="solaris"
	    ;;
    esac
    JDK_CFLAGS="$JDK_CFLAGS $JDK_CFLAGS/$SUN_EXTRA_INCLUDE"

    SAVE_CPPFLAGS=$CPPFLAGS
    CPPFLAGS="$CPPFLAGS $JDK_CFLAGS"
    AC_CHECK_HEADERS(jni.h)
    CPPFLAGS=$SAVE_CPPFLAGS

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
	JDK_LIBS="-Wl,-framework -Wl,JavaVM"
    elif test "X$ac_cv_header_jni_h" == "Xyes" ; then
	# Linux
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
AC_CHECK_PROGS(JAVAC, gcj javac ecj)
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
else
    AC_MSG_NOTICE([Missing java wrapper or swig - skipping java bindings])
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
else
    AC_MSG_NOTICE([Missing python wrapper or swig - skipping python bindings])
fi

if test "X$PYTHONWRAP" = "Xyes" && test "X$PYTHON" != "X" && test "X$enable_python" = "Xyes"; then
    AC_MSG_NOTICE([Python available: $PYTHON_VERSION])
else
    enable_python=no
fi

AC_MSG_NOTICE([Python enabled: $enable_python])

AM_CONDITIONAL([PYTHON_ENABLED],[test "x$enable_python" = "xyes"])
AM_CONDITIONAL([PYTHON_SERVICE],[test ! "x$PYTHON_VERSION" '<' "x2.4"])

AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "x$PKG_CONFIG" = "xno"; then
        AC_MSG_ERROR([ *** pkg-config not found])
else
        pkgconfigdir=${libdir}/pkgconfig
        AC_SUBST(pkgconfigdir)
fi
docdir='${datadir}/doc/${PACKAGE}'
AC_SUBST(docdir)

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
AC_CHECK_HEADER([glibmm/optioncontext.h], [
  AC_TRY_COMPILE([#include <glibmm/optioncontext.h>],
    [Glib::OptionContext ctx; ctx.set_summary("summary")], [
      AC_DEFINE(HAVE_GLIBMM_OPTIONCONTEXT_SET_SUMMARY,,
        [define if glibmm has Glib::OptionContext::set_summary()])
      AC_MSG_NOTICE([using glibmm command line parsing])
    ], [
      AC_MSG_NOTICE([using getopt_long command line parsing])
    ]
  )
])
AC_TRY_COMPILE([#include <glibmm.h>],[Glib::SignalChildWatch watch = Glib::signal_child_watch();],[glibmm_childwatch=yes],[glibmm_childwatch=no])
if test "$glibmm_childwatch" = yes; then
  AC_DEFINE(HAVE_GLIBMM_CHILDWATCH,,[define if glibmm have support for controling state of children processes])
else
  AC_MSG_NOTICE([WARNING: glibmm has no API for controlling children processes - result of external processes may be inconsistent])
fi
AC_TRY_COMPILE([#include <glibmm.h>],[Glib::ModuleFlags flags = Glib::MODULE_BIND_LOCAL;],[glibmm_bind_local=yes],[glibmm_bind_local=no])
if test "$glibmm_bind_local" = yes; then
  AC_DEFINE(HAVE_GLIBMM_BIND_LOCAL,,[define if glibmm have support local symbol resolution in shared libraries])
else
  AC_MSG_NOTICE([WARNING: glibmm has no way to limit scope of symbols of shared libraries. Make sure external libraries used by plugins have no conflicting symbols. HINT: use Globus compiled against system OpenSSL library.])
fi
AC_TRY_COMPILE([#include <glibmm/miscutils.h>],[Glib::getenv("");],[glibmm_getenv=yes],[glibmm_getenv=no])
if test "$glibmm_getenv" = yes; then
  AC_DEFINE(HAVE_GLIBMM_GETENV,,[define if glibmm have getenv operations])
else
  AC_MSG_NOTICE([WARNING: glibmm has no support for getenv. Usage of libc getenv is unsafe in multi-threaded applications.])
fi
AC_TRY_COMPILE([#include <glibmm/miscutils.h>],[Glib::setenv("", "");],[glibmm_setenv=yes],[glibmm_setenv=no])
if test "$glibmm_setenv" = yes; then
  AC_DEFINE(HAVE_GLIBMM_SETENV,,[define if glibmm have setenv operations])
else
  AC_MSG_NOTICE([WARNING: glibmm has no support for setenv. Usage of libc setenv may be unsafe in multi-threaded applications.])
fi
AC_TRY_COMPILE([#include <glibmm/miscutils.h>],[Glib::unsetenv("");],[glibmm_unsetenv=yes],[glibmm_unsetenv=no])
if test "$glibmm_unsetenv" = yes; then
  AC_DEFINE(HAVE_GLIBMM_UNSETENV,,[define if glibmm have unsetenv operations])
else
  AC_MSG_NOTICE([WARNING: glibmm has no support for unsetenv. Usage of libc unsetenv may be unsafe in multi-threaded applications.])
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
"$PKG_CONFIG" openssl
if test "$?" = '1'; then
    if test "x$OPENSSL_BIN" != "x"; then
        OPENSSL_VERSION=`$OPENSSL_BIN version | cut -d' ' -f2`
        AC_MSG_NOTICE([Found OpenSSL version $OPENSSL_VERSION])
        OPENSSL_V1=`echo $OPENSSL_VERSION | cut -d'.' -f2`
        OPENSSL_V2=`echo $OPENSSL_VERSION | cut -d'.' -f3 | tr -d [[:alpha:]]`
        OPENSSL_V3=`echo $OPENSSL_VERSION | cut -d'.' -f3 | tr -d [[:digit:]] | od -td1 | awk 'NR==1{print $2}'`
        OPENSSL_PROXY=no
        if test "$OPENSSL_V1" -ge "9" && test "$OPENSSL_V2" -ge "7" && test "$OPENSSL_V3" -ge "102"; then
            OPENSSL_PROXY=yes
            AC_MSG_NOTICE([Found OpenSSL with PROXY support])
        else
            AC_MSG_NOTICE([WARNING: OpenSSL does not contain PROXY support])
        fi
        OPENSSL_X509_VERIFY_PARAM=no
        if test "$OPENSSL_V1" -ge "9" && test "$OPENSSL_V2" -ge "8"; then
            OPENSSL_X509_VERIFY_PARAM=yes
            AC_MSG_NOTICE([Found OpenSSL with X509_VERIFY_PARAM])
        else
            AC_MSG_NOTICE([WARNING: OpenSSL does not contain X509_VERIFY_PARAM])
        fi
        OPENSSL_OLDRSA=yes
        if test "$OPENSSL_V1" -ge "9" && test "$OPENSSL_V2" -ge "8" && test "x$OPENSSL_V3" != "x"; then
            AC_MSG_NOTICE([No old RSA])
            OPENSSL_OLDRSA=no
        fi
    else
        AC_ERROR([Cannot determine OpenSSL version!])
    fi
else
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
    "$PKG_CONFIG" --exists 'openssl >= 0.9.8'
    if test "$?" = '0' ; then
            AC_MSG_NOTICE([No old RSA])
            OPENSSL_OLDRSA=no
    fi
fi

# check cppunit
PKG_CHECK_MODULES(CPPUNIT, [cppunit],[],
[AC_PATH_PROG(CPPUNIT_CONFIG, cppunit-config, no)
if test "x$CPPUNIT_CONFIG" = "xno"; then
        AC_MSG_WARN([cppunit-config not found - no UNIT testing will be performed])
        CPPUNIT_CFLAGS=
        CPPUNIT_LIBS=
else
        CPPUNIT_CFLAGS="`$CPPUNIT_CONFIG --cflags`"
        CPPUNIT_LIBS="`$CPPUNIT_CONFIG --libs`"
fi])
if test "x$CPPUNIT_CONFIG" != "xno" || test "x$CPPUNIT_PKG_ERRORS" != "x"
then
  enable_cppunit=yes
  TEST_DIR=test
else
  enable_cppunit=no
  TEST_DIR=
fi
AC_SUBST(CPPUNIT_CFLAGS)
AC_SUBST(CPPUNIT_LIBS)
AC_SUBST(TEST_DIR)

#check lasso
#LASSO_INSTALLED=no
#PKG_CHECK_MODULES(LASSO, [lasso >= 2.1.1], 
#[AC_SUBST(LASSO_CFLAGS) 
# AC_SUBST(LASSO_LIBS)
# LASSO_INSTALLED=yes],
#[])

##############################
#
# Check xmlsec1
#
#############################
XMLSEC_MIN_VERSION="1.2.4"
XMLSEC_OPENSSL_MIN_VERSION="1.2.4"
XMLSEC_CONFIG="xmlsec1-config"
XMLSEC_CFLAGS=""
XMLSEC_LIBS=""
XMLSEC_INSTALLED=no
dnl Check if xmlsec1 is explicitly disabled, default is enable.
AC_ARG_ENABLE(xmlsec1, AC_HELP_STRING([--disable-xmlsec1], [disable the xmlsec1 library, default is enable]),[disable_xmlsec1="yes"],[disable_xmlsec1="no"])

if test "x$disable_xmlsec1" = "xno"; then
  AC_ARG_WITH(xmlsec1, [  --with-xmlsec1=(PATH)  xmlsec1 location])
  if test "x$with_xmlsec1" = "x" ; then
    PKG_CHECK_MODULES(XMLSEC, [xmlsec1 >= $XMLSEC_MIN_VERSION xmlsec1-openssl >= XMLSEC_OPENSSL_MIN_VERSION],
        [(PKG_CHECK_MODULES(XMLSEC1_OPENSSL, [xmlsec1-openssl >= $XMLSEC_OPENSSL_MIN_VERSION], 
          [XMLSEC_INSTALLED=yes],[XMLSEC_INSTALLED=no]))],
	[XMLSEC_INSTALLED=no])
  fi

  if test "x$XMLSEC_INSTALLED" = "xno" ; then
      AC_MSG_CHECKING(for xmlsec1 libraries >= $XMLSEC_MIN_VERSION) 
      if test "x$with_xmlsec1" != "x" ; then 
   	  XMLSEC_CONFIG=$with_xmlsec1/bin/$XMLSEC_CONFIG
      fi
      if ! $XMLSEC_CONFIG --version > /dev/null 2>&1 ; then
	  AC_MSG_NOTICE(Could not find xmlsec1 anywhere; The xml security related functionality will not be compiled)
      fi
      vers=`$XMLSEC_CONFIG --version | awk -F. '{ printf "%d", ($1 * 1000 + $2) * 1000 + $3;}'`
      minvers=`echo $XMLSEC_MIN_VERSION | awk -F. '{ printf "%d", ($1 * 1000 + $2) * 1000 + $3;}'`
      if test "$vers" -ge "$minvers" ; then
          XMLSEC_LIBS="`$XMLSEC_CONFIG --libs`"
          XMLSEC_CFLAGS="`$XMLSEC_CONFIG --cflags`"
	  XMLSEC_INSTALLED="yes"
          AC_MSG_RESULT(yes ('$XMLSEC_VERSION'))
      else
          AC_MSG_NOTICE(You need at least xmlsec1 $XMLSEC_MIN_VERSION for this version of arc1)
      fi
  fi

  AC_SUBST(XMLSEC_CFLAGS)
  AC_SUBST(XMLSEC_LIBS)
  AC_SUBST(XMLSEC1_OPENSSL_CFLAGS)
  AC_SUBST(XMLSEC1_OPENSSL_LIBS)

  #AC_SUBST(XMLSEC_CONFIG)
  #AC_SUBST(XMLSEC_MIN_VERSION)
fi


#########################
#
# Check libmysqlclient
#
#########################
MYSQL_CONFIG="mysql_config"
MYSQL_CFLAGS=""
MYSQL_LIBS=""
MYSQL_INSTALLED=no
dnl Check if libmysqlclient is explicitly enabled, default is disable.
AC_ARG_ENABLE(mysql, AC_HELP_STRING([--enable-mysql], [enable the MySQL client library]),[enable_mysql="yes"],[enable_mysql="no"])
# Ask user for path to libmysqlclient
if test "x$enable_mysql" = "xyes"; then
  AC_ARG_WITH(mysql, [  --with-mysql=(PATH) prefix of MySQL installation. e.g. /usr/local or /usr])
  AC_MSG_CHECKING(for mysql client library)
  if test "x$with_mysql" != "x" ; then
      MYSQL_CONFIG=$with_mysql/bin/$MYSQL_CONFIG
  fi
  if ! $MYSQL_CONFIG --version > /dev/null 2>&1 ; then
      AC_MSG_ERROR(Could not find mysql C library anywhere (see config.log for details).)
  fi
  MYSQL_LIBS="`$MYSQL_CONFIG --libs`"
  MYSQL_CFLAGS="`$MYSQL_CONFIG --cflags`"
  MYSQL_INSTALLED="yes"

  AC_SUBST(MYSQL_LIBS)
  AC_SUBST(MYSQL_CFLAGS)
fi
AC_MSG_NOTICE([MySQL client library enabled: $MYSQL_INSTALLED])
AM_CONDITIONAL([MYSQL_LIBRARY_ENABLED],[test "x$MYSQL_INSTALLED" = "xyes"])


# check zlib 
AC_ARG_WITH(zlib,
  AC_HELP_STRING([--with-zlib=PATH], [where zlib is installed]),
    [ if test -d "$withval"; then 
         CPPFLAGS="${CPPFLAGS} -I$withval/include"
	 LDFLAGS="${LDFLAGS} -L$withval/lib"
      fi
    ]
)
AC_CHECK_HEADER([zlib.h],,AC_MSG_ERROR([unable to find zlib header files]))
AC_CHECK_LIB([z],[deflateInit2_],,AC_MSG_ERROR([unable to link with zlib library]))


###############################################
#
# Check for Berkeley DB C++
#
###############################################

DBCXX_CPPFLAGS=

#
# Allow the user to specify db_cxx.h location (we will still check though)
#

dbcxx_include_paths=
AC_ARG_WITH(dbcxx-include,
   [  --with-dbcxx-include=PATH   Specify path to db_cxx.h],
   [
	if test "x$withval" = "xyes" ; then
		AC_MSG_ERROR([--with-dbcxx-include requires PATH argument])
	fi
	if test "x$withval" != "xno" ; then
		dbcxx_include_paths=$withval
	fi
   ]
)

AC_LANG_SAVE
AC_LANG_CPLUSPLUS

#
# If user did not specify location we start by searching at the standard locations
#

if test x$dbcxx_include_paths = x
then
	AC_MSG_NOTICE([Looking for db_cxx.h in standard locations]) 
	AC_CHECK_HEADERS(db_cxx.h,HAVE_DBCXX=yes,HAVE_DBCXX=no)
	# If the user did not provide a location we have some good suggestions
	dbcxx_include_paths="/usr/include/db4 /usr/include/db44"
else
	HAVE_DBCXX=no
fi

#
# Now Look for db_cxx.h in non-standard locations
#

if test "$HAVE_DBCXX" = no
then
  for dbcxx_dir in $dbcxx_include_paths
  do
    SAVE_CPPFLAGS=$CPPFLAGS
    DBCXX_CPPFLAGS=-I$dbcxx_dir
    CPPFLAGS="$CPPFLAGS $DBCXX_CPPFLAGS"
    # Disable Autoconf caching
    unset ac_cv_header_db_cxx_h
    AC_MSG_NOTICE([Looking for db_cxx.h in $dbcxx_dir]) 
    AC_CHECK_HEADERS(db_cxx.h,HAVE_DBCXX=yes,HAVE_DBCXX=no)
    CPPFLAGS=$SAVE_CPPFLAGS
    # If a db_cxx.h was found we break and keep the current value of DBCXX_CPPFLAGS
    if test "$HAVE_DBCXX" = yes
    then
      break   
    fi
    DBCXX_CPPFLAGS=
  done
fi
AC_SUBST(DBCXX_CPPFLAGS)

if test "$HAVE_DBCXX" = no
then
	DBCXX_LIBS=""
else
	# pthread needed for RH9
	SAVE_LDFLAGS=$LDFLAGS
	LDFLAGS="$LDFLAGS -lpthread"

	AC_CHECK_LIB(db_cxx,main,DBCXX_LIBS="-ldb_cxx",DBCXX_LIBS="") 

	if test "$DBCXX_LIBS" = ""
	then
		AC_MSG_WARN([BerkeleyDB library libdb_cxx was not found!])
		AC_CHECK_LIB(db_cxx-4.2,main,DBCXX_LIBS="-ldb_cxx-4.2",DBCXX_LIBS="") 
	fi

	if test "$DBCXX_LIBS" = ""
	then
		AC_MSG_WARN([BerkeleyDB library libdb_cxx-4.2 was not found!])
	fi

	if test "$DBCXX_LIBS" = ""
	then
		AC_MSG_WARN([No BerkeleyDB library found!])
	fi
	LDFLAGS=$SAVE_LDFLAGS
fi

AM_CONDITIONAL([DBXML_ENABLED],[test ! "x$DBCXX_LIBS" = "x"])
AC_SUBST(DBCXX_LIBS)
if test ! "x$DBCXX_LIBS" = "x"
then
  AC_DBCXX_HAVE_DBDEADLOCKEXCEPTION
fi
AC_LANG_RESTORE

# globus/gpt packages
GPT_PKG(globus_rls_client)
AC_SUBST(GLOBUS_RLS_CLIENT_CFLAGS)
AC_SUBST(GLOBUS_RLS_CLIENT_LIBS)
GPT_PKG(globus_ftp_client)
AC_SUBST(GLOBUS_FTP_CLIENT_CFLAGS)
AC_SUBST(GLOBUS_FTP_CLIENT_LIBS)
GPT_PKG(globus_ftp_control)
AC_SUBST(GLOBUS_FTP_CONTROL_CFLAGS)
AC_SUBST(GLOBUS_FTP_CONTROL_LIBS)
GPT_PKG(globus_rsl)
AC_SUBST(GLOBUS_RSL_CFLAGS)
AC_SUBST(GLOBUS_RSL_LIBS)
GPT_PKG(globus_io)
AC_SUBST(GLOBUS_IO_CFLAGS)
AC_SUBST(GLOBUS_IO_LIBS)

# Check for gridftp-v2
SAVE_CFLAGS=$CFLAGS
SAVE_LIBS=$LIBS
CFLAGS="$CFLAGS $GLOBUS_FTP_CLIENT_CFLAGS"
LIBS="$LIBS $GLOBUS_FTP_CLIENT_LIBS"
AC_CHECK_FUNCS(globus_ftp_client_handleattr_set_gridftp2)
CFLAGS=$SAVE_CFLAGS
LIBS=$SAVE_LIBS

if test "x$GLOBUS_IO_VERSION" = "x"; then
  IO_VERSION_MAJOR=0
else
  IO_VERSION_MAJOR=`echo "$GLOBUS_IO_VERSION" | sed 's/^\([[^.]]\).*/\1/'`;
fi
AC_DEFINE_UNQUOTED(GLOBUS_IO_VERSION,$IO_VERSION_MAJOR,[Globus IO version])

# Check for LFC
AC_CHECK_HEADER(lfc/lfc_api.h,[LFC=yes],[LFC=no])

# Check if user asks to skip documentation build
AC_ARG_ENABLE(doc, AC_HELP_STRING([--disable-doc], [disable building documentation (requires doxygen and pdflatex)]),[],enable_doc="yes")
if test "x$enable_doc" = "xyes"; then
  AC_CHECK_PROGS(PDFLATEX, pdflatex)
  AC_CHECK_PROGS(DOXYGEN, doxygen)
  if test "x$PDFLATEX" = "x"; then
    enable_doc="no"
    AC_MSG_NOTICE([WARNING: Missing pdflatex - documentation won't be built])
  elif test "x$DOXYGEN" = "x"; then
    enable_doc="no"
    AC_MSG_NOTICE([WARNING: Missing doxygen - documentation won't be built])
  fi
fi
AC_MSG_NOTICE([Documentation enabled: $enable_doc])
AM_CONDITIONAL([DOC_ENABLED],[test "x$enable_doc" = "xyes"])

# Setup conditionals
AM_CONDITIONAL([RLS_ENABLED], test -n "$GLOBUS_RLS_CLIENT_VERSION")
AM_CONDITIONAL([GRIDFTP_ENABLED], test -n "$GLOBUS_FTP_CLIENT_VERSION")
AM_CONDITIONAL([LFC_ENABLED], test x$LFC = xyes)
AM_CONDITIONAL([RSL_ENABLED], test -n "$GLOBUS_RSL_VERSION")
AM_CONDITIONAL([OPENSSL_PROXY_ENABLED], test x$OPENSSL_PROXY = xyes)
AM_CONDITIONAL([OPENSSL_X509_VERIFY_PARAM_ENABLED], test x$OPENSSL_X509_VERIFY_PARAM = xyes)
AM_CONDITIONAL([XMLSEC_ENABLED], test x$XMLSEC_INSTALLED = xyes)

# Setup defines
if test -n "$GLOBUS_RSL_VERSION"; then
    AC_DEFINE(HAVE_GLOBUS_RSL,,[define if GLOBUS RSL package is available])
fi
if test -n "$GLOBUS_RLS_CLIENT_VERSION"; then
    AC_DEFINE(HAVE_GLOBUS_RLS_CLIENT,,[define if GLOBUS RSL package is available])
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
if test x"$XMLSEC_INSTALLED" = xyes; then
    AC_DEFINE(HAVE_XMLSEC,,[define if XMLSEC package is available])
fi

# The macro is used to set exec_prefix to it's expected
# value at the end of the configure script
AC_LIB_WITH_FINAL_PREFIX([
    eval instprefix="\"${exec_prefix}\""
    eval pkglibdir="\"${libdir}/${PACKAGE}\""
    eval pkglibexecdir="\"${libexecdir}/${PACKAGE}\""
])

instprefix=`echo "${instprefix}" | sed 's#/$##'`
pkglibsubdir=`echo "${pkglibdir}" | sed "s#^${instprefix}/##" \
    | sed -e 's#/$##' -e 's#^/##'`
pkglibexecsubdir=`echo "${pkglibexecdir}" | sed "s#^${instprefix}/##" \
    | sed -e 's#/$##' -e 's#^/##'`
inverse_pkglibexecsubdir=`echo "/${pkglibexecsubdir}/" \
    | perl -pwe 's#/.(?=/)##g; s#/+#/#g; s#/[^/]+(?=/)#/..#g' \
    | sed -e 's#/$##' -e 's#^/##'`
pkglibdir_rel_to_pkglibexecdir=`echo "${inverse_pkglibexecsubdir}/${pkglibsubdir}" \
    | sed -e 's#/$##' -e 's#^/##'`

AC_SUBST(instprefix)
AC_SUBST(pkglibsubdir)
AC_SUBST(pkglibexecdir)
AC_SUBST(pkglibexecsubdir)
AC_SUBST(pkglibdir_rel_to_pkglibexecdir)

AC_DEFINE_UNQUOTED(INSTPREFIX, ["${instprefix}"], [installation prefix])
AC_DEFINE_UNQUOTED(PKGLIBSUBDIR, ["${pkglibsubdir}"], [plugin installation subpath])
AC_DEFINE_UNQUOTED(PKGLIBEXECSUBDIR, ["${pkglibexecsubdir}"], [helper programs installation subpath])

# Setup messages for reporting
enable_rls=no
if test -n "$GLOBUS_RLS_CLIENT_VERSION" ; then enable_rls=yes; fi
enable_gridftp=no
if test -n "$GLOBUS_FTP_CLIENT_VERSION" ; then enable_gridftp=yes; fi
enable_lfc=no
if test x$LFC = xyes ; then enable_lfc=yes; fi
enable_rsl=no
if test -n "$GLOBUS_RSL_VERSION" ; then enable_rsl=yes; fi
enable_saml=no
if test x$XMLSEC_INSTALLED = xyes ; then enable_saml=yes; fi

# Check for LDAP
AC_CHECK_HEADER(ldap.h, [LDAP=yes], [LDAP=no])
SAVE_LDFLAGS=$LDFLAGS
LDFLAGS=-lpthread
AC_CHECK_LIB([ldap_r], [ldap_first_message], [ 
    AC_CHECK_LIB([ldap_r], [ldap_initialize], [ AC_DEFINE(HAVE_LDAP_INITIALIZE,[],[Define if you have ldap_initialize function]) ])  
    LDAP_LIBS=-lldap_r ], [
  AC_CHECK_LIB([ldap], [ldap_first_message], [ 
    AC_CHECK_LIB([ldap], [ldap_initialize], [ AC_DEFINE(HAVE_LDAP_INITIALIZE,[],[Define if you have ldap_initialize function]) ])  
    LDAP_LIBS=-lldap ], [
    LDAP=no
  ])
])
AC_CHECK_LIB([lber], [ber_init], [LDAP_LIBS="$LDAP_LIBS -llber"], []) 
AC_SUBST(LDAP_LIBS)
LDFLAGS=$SAVE_LDFLAGS

AM_CONDITIONAL([LDAP_ENABLED], test x$LDAP = xyes)

# Check for the uuid lib
UUID_LIBS=""
AC_CHECK_HEADER(uuid/uuid.h, [
  AC_CHECK_FUNC([uuid_generate], [UUID_LIBS=], [
    AC_CHECK_LIB([uuid], [uuid_generate], [UUID_LIBS=-luuid], [
      AC_MSG_NOTICE([Can't find library containing uuid implementation])
    ])
  ])
], [AC_MSG_NOTICE([Can't find uuid header])])
AC_SUBST(UUID_LIBS)
LIBS="$LIBS $UUID_LIBS"

# Check for dlopen
DLOPEN_LIBS=""
AC_CHECK_FUNC([dlopen], [DLOPEN_LIBS=], [
  AC_CHECK_LIB([dl], [dlopen], [DLOPEN_LIBS=-ldl], [
    AC_MSG_NOTICE([Can't find library containing dlopen implementation])
  ])
])
AC_SUBST(DLOPEN_LIBS)

# check for fsusage
gl_FSUSAGE

# Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([arpa/inet.h fcntl.h float.h limits.h netdb.h netinet/in.h sasl.h sasl/sasl.h stdint.h stdlib.h string.h sys/file.h sys/socket.h sys/vfs.h unistd.h uuid/uuid.h getopt.h])
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
AC_CHECK_FUNCS([acl bzero dup2 floor ftruncate gethostbyname_r gethostname getdomainname getpid gmtime_r lchown localtime_r memchr memmove memset mkdir mkfifo regcomp rmdir select sigwaitinfo setenv socket strcasecmp strchr strcspn strdup strerror strncasecmp strstr strtol strtoul strtoull timegm tzset unsetenv getopt_long_only])
AC_CHECK_LIB([resolv], [res_query], [LIBRESOLV=-lresolv], [LIBRESOLV=])
AC_CHECK_LIB([resolv], [__dn_skipname], [LIBRESOLV=-lresolv], [LIBRESOLV=])
AC_CHECK_LIB([nsl], [gethostbyname], [LIBRESOLV="$LIBRESOLV -lnsl"], [])
AC_CHECK_LIB([nsl], [getdomainname])
AC_SUBST(LIBRESOLV)


# check for platfom specific extra libraries and flags

EXTRA_LIBS=""
REGEX_LIBS=""
SOCKET_LIBS=""
WIN32=""
case "${host}" in
  *-*-mingw32)
    WIN32="yes"
    REGEX_LIBS="-lgnurx"
    SOCKET_LIBS="-L${PWD}/mingwlib -lws2_32"
    EXTRA_LIBS="-L${PWD}/mingwlib -lole32"
    # its required to libtool generate .dlls on win32 using mingw
    LDFLAGS="$LDFLAGS -no-undefined"
    # libtool hack. libtool which comes with mingw does not 
    # recognize {name}.dll libraries - it always reqires lib prefix.
    rm -rf ${PWD}/mingwlib
    mkdir -p ${PWD}/mingwlib
    cp ${SYSTEMROOT}/system32/ole32.dll ${PWD}/mingwlib/libole32.dll
    cp ${SYSTEMROOT}/system32/ws2_32.dll ${PWD}/mingwlib/libws2_32.dll
    ;;
  *solaris*)
    SOCKET_LIBS="-lsocket"
    CFLAGS="$CFLAGS -D_POSIX_PTHREAD_SEMANTICS"
    CXXFLAGS="$CXXFLAGS -D_POSIX_PTHREAD_SEMANTICS"
    ;;
esac
AC_SUBST(EXTRA_LIBS)
AC_SUBST(REGEX_LIBS)
AC_SUBST(SOCKET_LIBS)
AM_CONDITIONAL([WIN32], [ test "x$WIN32" == "xyes" ])

case " $LDFLAGS " in
     " -Wl,--no-undefined ") ;;
     " -Wl,-no-undefined ") ;;
     " -Wl,-z -Wl,defs ") ;;
     " -Wl,-z,defs ") ;;
     *)
        case "${host}" in
            *darwin*);;
            *solaris*);;
            *) LDFLAGS="$LDFLAGS -Wl,--no-undefined" ;;
        esac
        ;;
esac

# Check for explicitely and implicitely disabled services
if test "x$WIN32" = "xyes" ; then
  AC_MSG_NOTICE([In WIN32 environment many (all) services are not supported yet])
fi
# A-Rex
AC_ARG_ENABLE(a_rex_service, AC_HELP_STRING([--disable-a-rex-service], [disable building A-Rex service]),[],[
  if test ! "x$WIN32" = "xyes" ; then
    enable_a_rex_service="yes"
  else
    enable_a_rex_service="no"
  fi
])
AC_MSG_NOTICE([A-Rex service enabled: $enable_a_rex_service])
AM_CONDITIONAL([A_REX_SERVICE_ENABLED],[test "x$enable_a_rex_service" = "xyes"])

# ISI(S)
AC_ARG_ENABLE(isi_service, AC_HELP_STRING([--disable-isi-service], [disable building ISI Service]),[],[
  if test ! "x$WIN32" = "xyes" ; then
    enable_isi_service="yes"
  else
    enable_isi_service="no"
  fi
])
if test "$enable_isi_service" = "yes"; then
  if test "$DBCXX_LIBS" = ""; then
    enable_isi_service="no"
    AC_MSG_NOTICE([WARNING: Missing db_cxx - disabling ISI Service])
  fi
fi
AC_MSG_NOTICE([ISI Service enabled: $enable_isi_service])
AM_CONDITIONAL([ISI_SERVICE_ENABLED],[test "x$enable_isi_service" = "xyes"])

# Charon service
AC_ARG_ENABLE(charon_service, AC_HELP_STRING([--disable-charon-service], [disable building Charon Service]),[],[
  if test ! "x$WIN32" = "xyes" ; then
    enable_charon_service="yes"
  else
    enable_charon_service="no"
  fi
])
AC_MSG_NOTICE([Charon Service enabled: $enable_charon_service])
AM_CONDITIONAL([CHARON_SERVICE_ENABLED],[test "x$enable_charon_service" = "xyes"])

# Hopi service
AC_ARG_ENABLE(hopi_service, AC_HELP_STRING([--disable-hopi-service], [disable building Hopi Service]),[],[
  if test ! "x$WIN32" = "xyes" ; then
    enable_hopi_service="yes"
  else
    enable_hopi_service="no"
  fi
])
AC_MSG_NOTICE([Hopi Service enabled: $enable_hopi_service])
AM_CONDITIONAL([HOPI_SERVICE_ENABLED],[test "x$enable_hopi_service" = "xyes"])

# Paul service 
AC_ARG_ENABLE(paul_service, AC_HELP_STRING([--disable-paul-service], [disable building PAUL Service]),[],[
  if test ! "x$WIN32" = "xyes" ; then
    enable_paul_service="yes"
  else
    enable_paul_service="no"
  fi
])
AC_MSG_NOTICE([PAUL Service enabled: $enable_paul_service])
AM_CONDITIONAL([PAUL_SERVICE_ENABLED],[test "x$enable_paul_service" = "xyes"])

# Sched service
AC_ARG_ENABLE(sched_service, AC_HELP_STRING([--disable-sched-service], [disable building SCHED Service]),[],[
  if test ! "x$WIN32" = "xyes" ; then
    enable_sched_service="yes"
  else
    enable_sched_service="no"
  fi
])
if test "$enable_sched_service" = "yes"; then
  if test "$DBCXX_LIBS" = ""; then
    enable_sched_service="no"
    AC_MSG_NOTICE([WARNING: Missing db_cxx - disabling SCHED Service])
  fi
fi
AC_MSG_NOTICE([SCHED Service enabled: $enable_sched_service])
AM_CONDITIONAL([SCHED_SERVICE_ENABLED],[test "x$enable_sched_service" = "xyes"])

# Storage service
if test "x$enable_python" = "xyes"; then
    AC_ARG_ENABLE(storage_service, AC_HELP_STRING([--disable-storage-service], [disable building STORAGE Service]),[],enable_storage_service="yes")
    AC_MSG_NOTICE([STORAGE Service enabled: $enable_storage_service])
else
    enable_storage_service="no"
    AC_MSG_NOTICE([STORAGE Service disabled - Python needed])
fi
AM_CONDITIONAL([STORAGE_SERVICE_ENABLED],[test "x$enable_storage_service" = "xyes"])

# hack for backend scripts

pbs_bin_path=/usr/bin
pbs_log_path=/var/spool/pbs/server_logs
tmp_dir=/tmp
gnu_time=/usr/bin/time
nodename="/bin/hostname -f"
runtime_config_dir=
runtime_local_scratch_dir=
runtime_frontend_sees_node=
runtime_node_sees_frontend=
arc_location=$prefix

AC_SUBST(arc_location)
AC_SUBST(pbs_bin_path)
AC_SUBST(pbs_log_path)
AC_SUBST(tmp_dir)
AC_SUBST(gnu_time)
AC_SUBST(nodename)
AC_SUBST(runtime_config_dir)
AC_SUBST(runtime_local_scratch_dir)
AC_SUBST(runtime_frontend_sees_node)
AC_SUBST(runtime_node_sees_frontend)

# Shell for the job control scripts

case $host_os in

  solaris* )
      posix_shell='/usr/xpg4/bin/sh'
      ;;
  * )
      posix_shell='/bin/sh'
      ;;
esac

AC_SUBST(posix_shell)

DATE=`date +%Y-%m-%d`
AC_SUBST(DATE)

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/hed/Makefile
                 src/hed/libs/client/Makefile
                 src/hed/libs/common/Makefile
                 src/hed/libs/common/test/Makefile
                 src/hed/libs/credential/Makefile
                 src/hed/libs/data/Makefile
                 src/hed/libs/data/cache-clean.1
                 src/hed/libs/data/cache-list.1
                 src/hed/libs/data/test/Makefile
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
                 src/hed/libs/ws-security/Makefile
                 src/hed/libs/infosys/Makefile
                 src/hed/libs/delegation/Makefile
                 src/hed/libs/ws/Makefile
                 src/hed/libs/dbxml/Makefile
                 src/hed/daemon/Makefile
                 src/hed/daemon/scripts/Makefile
                 src/hed/daemon/unix/Makefile
                 src/hed/daemon/win32/Makefile
                 src/hed/mcc/Makefile
                 src/hed/mcc/soap/Makefile
                 src/hed/mcc/tcp/Makefile
                 src/hed/mcc/http/Makefile
                 src/hed/mcc/tls/Makefile
                 src/hed/mcc/msgvalidator/Makefile
                 src/hed/acc/Makefile
                 src/hed/acc/ARC0/Makefile
                 src/hed/acc/ARC1/Makefile
                 src/hed/acc/CREAM/Makefile
                 src/hed/acc/Broker/Makefile
                 src/hed/dmc/Makefile
                 src/hed/dmc/file/Makefile
                 src/hed/dmc/gridftp/Makefile
                 src/hed/dmc/http/Makefile
                 src/hed/dmc/ldap/Makefile
                 src/hed/dmc/lfc/Makefile
                 src/hed/dmc/rls/Makefile
                 src/hed/pdc/Makefile
                 src/hed/pdc/arcpdp/Makefile
                 src/hed/pdc/gaclpdp/Makefile
                 src/hed/pdc/pdpserviceinvoker/Makefile
                 src/hed/pdc/allowpdp/Makefile
                 src/hed/pdc/denypdp/Makefile
                 src/hed/pdc/countpdp/Makefile
                 src/hed/pdc/simplelistpdp/Makefile
                 src/hed/pdc/arcauthzsh/Makefile
                 src/hed/pdc/delegationsh/Makefile
                 src/hed/pdc/usernametokensh/Makefile
                 src/hed/pdc/x509tokensh/Makefile
                 src/hed/identitymap/Makefile
                 src/services/Makefile
                 src/services/a-rex/Makefile
                 src/services/a-rex/arex_service.xml.example
                 src/services/a-rex/arex_service-secure.xml.example		 
                 src/services/a-rex/config/Makefile
                 src/services/a-rex/grid-manager/Makefile
                 src/services/a-rex/grid-manager/conf/Makefile
                 src/services/a-rex/grid-manager/files/Makefile
                 src/services/a-rex/grid-manager/jobdesc/Makefile
                 src/services/a-rex/grid-manager/jobdesc/jsdl/Makefile
                 src/services/a-rex/grid-manager/jobdesc/rsl/Makefile
                 src/services/a-rex/grid-manager/jobs/Makefile
                 src/services/a-rex/grid-manager/loaders/Makefile
                 src/services/a-rex/grid-manager/log/Makefile
                 src/services/a-rex/grid-manager/mail/Makefile
                 src/services/a-rex/grid-manager/misc/Makefile
                 src/services/a-rex/grid-manager/run/Makefile
                 src/services/a-rex/infoproviders/Makefile
                 src/services/a-rex/infoproviders/CEinfo.pl
                 src/services/a-rex/ldif/Makefile
                 src/services/a-rex/lrms/Makefile
                 src/services/a-rex/lrms/submit_common.sh
                 src/services/a-rex/lrms/condor/Makefile
                 src/services/a-rex/lrms/condor/finish-condor-job
                 src/services/a-rex/lrms/condor/cancel-condor-job
                 src/services/a-rex/lrms/condor/submit-condor-job
                 src/services/a-rex/lrms/fork/Makefile
                 src/services/a-rex/lrms/fork/scan-fork-job
                 src/services/a-rex/lrms/fork/submit-fork-job
                 src/services/a-rex/lrms/fork/cancel-fork-job
                 src/services/a-rex/lrms/ll/Makefile
                 src/services/a-rex/lrms/ll/submit-ll-job
                 src/services/a-rex/lrms/lsf/Makefile
                 src/services/a-rex/lrms/lsf/submit-lsf-job
                 src/services/a-rex/lrms/pbs/Makefile
                 src/services/a-rex/lrms/pbs/submit-pbs-job
                 src/services/a-rex/lrms/pbs/cancel-pbs-job
                 src/services/a-rex/lrms/pbs/scan-pbs-job
                 src/services/a-rex/lrms/sge/Makefile
                 src/services/a-rex/lrms/sge/submit-sge-job
                 src/services/a-rex/lrms/sge/scan-sge-job
                 src/services/a-rex/lrms/sge/cancel-sge-job
                 src/services/charon/Makefile
                 src/services/charon/charon_service.xml.example
                 src/services/hopi/Makefile
                 src/services/hopi/hopi_service.xml.example
                 src/services/arex2/Makefile
                 src/services/isis/Makefile
                 src/services/saml/Makefile
                 src/services/sched/Makefile
                 src/services/sched/manage_jobq.8
                 src/services/paul/Makefile
                 src/services/storage/Makefile 
                 src/services/storage/storage_service.xml.example
                 src/services/storage/arc_storage_cli.1
                 src/services/storage/librarian/Makefile 
                 src/services/storage/cli/Makefile 
                 src/services/storage/shepherd/Makefile 
                 src/services/storage/ahash/Makefile 
                 src/services/storage/bartender/Makefile 
                 src/services/storage/store/Makefile 
                 src/services/echo_java/Makefile
                 src/services/echo_python/Makefile
                 src/tests/echo/echo_service.xml.example
                 src/services/wrappers/Makefile
                 src/services/wrappers/java/Makefile
                 src/services/wrappers/python/Makefile
                 src/clients/Makefile
                 src/clients/data/Makefile
                 src/clients/data/arccp.1
                 src/clients/data/arcls.1
                 src/clients/data/arcrm.1
                 src/clients/echo/Makefile
                 src/clients/echo/arcecho.1
                 src/clients/echo/echo_client.xml.example
                 src/clients/charon/Makefile
                 src/clients/charon/arcdecision.1
                 src/clients/isis/Makefile
                 src/clients/wsrf/Makefile
                 src/clients/wsrf/arcinfo.1
                 src/clients/credentials/Makefile
                 src/clients/credentials/arcproxy.1
                 src/clients/voms/Makefile
                 src/clients/voms/voms_assertion_init.1
                 src/clients/arclib/Makefile
                 src/clients/arclib/arcstat.1
                 src/clients/arclib/arcsub.1
                 src/clients/arclib/arcclean.1
                 src/clients/arclib/arckill.1
                 src/clients/arclib/arcget.1
                 src/clients/arclib/arccat.1
                 src/clients/arclib/arcclient.xml.5
                 src/tests/Makefile
                 src/tests/data/Makefile
                 src/tests/echo/Makefile
                 src/tests/echo/perftest.1
                 src/tests/policy-delegation/Makefile
                 src/tests/translator/Makefile
                 src/tests/xpath/Makefile
                 src/tests/arcpolicy/Makefile
                 src/utils/wsdl2hed.1
                 src/utils/Makefile
                 swig/Makefile
                 java/Makefile
                 python/Makefile
                 po/Makefile.in
                 doc/Makefile
                 include/Makefile
                 debian/Makefile
                 debian/nordugrid-arc1-client.install
                 debian/nordugrid-arc1-server.install
                 debian/nordugrid-arc1-java.install
                 debian/nordugrid-arc1-lib.install
                 debian/nordugrid-arc1-python.install
                 debian/nordugrid-arc1-plugins-base.install
                 arc.spec
                 src/hed/daemon/arched.8
])
AC_OUTPUT

AC_MSG_RESULT([
Unit testing:       ${enable_cppunit}
Java binding:       ${enable_java}
Python binding:     ${enable_python} ($PYTHON_VERSION)

Available third-party features:

RLS:                ${enable_rls}
GridFTP:            ${enable_gridftp}
LFC:                ${enable_lfc}
RSL:                ${enable_rsl}
SAML:               ${enable_saml}
MYSQL CLIENT LIB:   ${enable_mysql}

Included components:
A-Rex service:      ${enable_a_rex_service}
ISI service:        ${enable_isi_service}
CHARON service:     ${enable_charon_service}
HOPI service:       ${enable_hopi_service}
SCHED service:      ${enable_sched_service}
STORAGE service:    ${enable_storage_service}
PAUL service:       ${enable_paul_service}
])

if test ! "X$OPENSSL_PROXY" = "Xyes"; then
AC_MSG_RESULT([

OpenSSL contains no support for proxy credentials.
You will have to use certificate/private key pairs directly.
Make sure private keys are not protected by password before
using them.
It is advisable to update OpenSSL version to at least 0.9.7g
to avoid this situation.

])
fi
