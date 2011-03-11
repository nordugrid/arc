#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.56)
AC_CANNONICAL_SYSTEM
AC_INIT([nordugrid-arc],[1.0.0],[http://bugzilla.nordugrid.org/])
AM_INIT_AUTOMAKE([foreign tar-pax])
AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])

baseversion=`echo $VERSION | sed 's/[[^0-9.]].*//'`
preversion=`echo $VERSION | sed 's/^[[0-9.]]*//'`

if test "x$preversion" = "x" ; then
   fedorarelease="1"
   fedorasetupopts="-q"
   debianversion="$baseversion"
else
   fedorarelease="0.$preversion"
   fedorasetupopts="-q -n %{name}-%{version}$preversion"
   debianversion="$baseversion~$preversion"
fi

AC_SUBST(baseversion)
AC_SUBST(preversion)
AC_SUBST(fedorarelease)
AC_SUBST(fedorasetupopts)
AC_SUBST(debianversion)

# This macro was introduced in autoconf 2.57g? but we currently only require 2.56
m4_ifdef([AC_CONFIG_MACRO_DIR], [AC_CONFIG_MACRO_DIR([m4])])

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

dnl --with-docdir since older autoconf (<2.60) does not support --docdir
AC_ARG_WITH([docdir],
AC_HELP_STRING([--with-docdir=DIR],
  [Install documentation in DIR [[default: ${datadir}/doc]]]),
  [case $withval in
    yes|no)
      AC_MSG_ERROR([Invalid DIR])
      ;;
    *)
      docdir="$withval"
      ;;
  esac
],
[
  if test -z "$docdir"; then
    docdir='${datadir}/doc/${PACKAGE}'
  fi
])
AC_SUBST(docdir)

# Use arc for "pkgdir" instead of nordugrid-arc (@PACKAGE@)
pkgdatadir='${datadir}/arc'
pkgincludedir='${includedir}/arc'
pkglibdir='${libdir}/arc'
pkglibexecdir='${libexecdir}/arc'

AC_SUBST(pkgdatadir)
AC_SUBST(pkgincludedir)
AC_SUBST(pkglibdir)
AC_SUBST(pkglibexecdir)

ARC_API
ARC_RELATIVE_PATHS

AC_ARG_WITH(sysv-scripts-location,
  AC_HELP_STRING([--with-sysv-scripts-location=<PATH>], [Location of the SYSV init scripts. [[autodetect]]]),
  [ initddir="$withval" ],
  [
    initddir=
    case "${host}" in
      *linux*)
        for i in init.d rc.d/init.d rc.d; do
          if test -d "/etc/$i" -a ! -h "/etc/$i" ; then
            initddir="$sysconfdir/$i"
            break
          fi
        done

        if test -z "$initddir"; then
          AC_MSG_WARN(could not find a suitable location for the SYSV init scripts - not installing)
        fi
        ;;
    esac
  ]
)
AM_CONDITIONAL([SYSV_SCRIPTS_ENABLED],[test "x$initddir" != "x"])
AC_MSG_RESULT($initddir)
AC_SUBST(initddir)

AC_ARG_WITH(cron-scripts-prefix,
  AC_HELP_STRING([--with-cron-scripts-prefix=<PATH>], [Specify the location of the cron directory. [[SYSCONFDIR/cron.d]]]),
  [ cronddir="$withval" ],
  [ cronddir="$sysconfdir/cron.d" ]
)
AC_SUBST(cronddir)

# gettext
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.12])
[[ -e po/POTFILES.in ]] || touch po/POTFILES.in

# Portable 64bit file offsets
AC_SYS_LARGEFILE

# pkg-config needed for many checks
AC_PATH_PROG(PKG_CONFIG, pkg-config, no)
if test "x$PKG_CONFIG" = "xno"; then
        AC_MSG_ERROR([ *** pkg-config not found])
else
        pkgconfigdir=${libdir}/pkgconfig
        AC_SUBST(pkgconfigdir)
fi

# Default enable/disable switches
#  Features
enables_ldap=yes
enables_mysql=no
enables_swig=yes
# Features directly related to components
enables_cppunit=yes
enables_java=yes
enables_python=yes
enables_lfc=yes
enables_gacl=yes
enables_argus=no
enables_xmlsec1=yes
#  Libraries and plugins
# Currently no fine-grained choice is supported.
# Also this variable is used to check if source
# build is neded at all because no component can
# be built without HED.
enables_hed=yes
#  Services
enables_a_rex_service=yes
enables_gridftpd_service=yes
enables_hopi_service=yes
enables_isis_service=yes
enables_ldap_service=yes
enables_giis_service=yes
enables_janitor_service=yes
#enables_charon_service=yes
enables_storage_service=yes
enables_cache_service=yes
enables_ldap_monitor=yes
enables_ws_monitor=yes
#  Clients
enables_arclib_client=yes
enables_charon_client=yes
enables_credentials_client=yes
enables_echo_client=yes
enables_data_client=yes
enables_isis_client=yes
enables_jura_client=yes
enables_saml_client=yes
enables_srm_client=yes
enables_wsrf_client=yes
enables_cache_client=yes
enables_unicore_client=no
#  Documentation
enables_doc=yes

# Handle group enable/disable switches
AC_ARG_ENABLE(all, AC_HELP_STRING([--disable-all], [disables all buildable components. Can be overwritten with --enable-* for group or specific component. It is also possible to use --enable-all to overwrite defaults.]),
[
enables_a_rex_service=$enableval
enables_gridftpd_service=$enableval
enables_hopi_service=$enableval
enables_isis_service=$enableval
enables_ldap_service=$enableval
enables_giis_service=$enableval
enables_ldap_monitor=$enableval
enables_ws_monitor=$enableval
enables_janitor_service=$enableval
#enables_charon_service=$enableval
enables_storage_service=$enableval
enables_cache_service=$enableval
enables_arclib_client=$enableval
enables_charon_client=$enableval
enables_credentials_client=$enableval
enables_echo_client=$enableval
enables_data_client=$enableval
enables_isis_client=$enableval
enables_jura_client=$enableval
enables_saml_client=$enableval
enables_srm_client=$enableval
enables_wsrf_client=$enableval
enables_cache_client=$enableval
enables_unicore_client=$enableval
enables_hed=$enableval
enables_java=$enableval
enables_python=$enableval
enables_gacl=$enableval
enables_lfc=$enableval
enables_xmlsec1=$enableval
enables_argus=$enableval
enables_cppunit=$enableval
enables_doc=$enableval
],
[])

AC_ARG_ENABLE(all-clients, AC_HELP_STRING([--disable-all-clients], [disables all buildable client components. Can be overwritten with --enable-* for specific component. It is also possible to use --enable-all-clients to overwrite defaults and --enable-all.]),
[
enables_arclib_client=$enableval
enables_charon_client=$enableval
enables_credentials_client=$enableval
enables_echo_client=$enableval
enables_data_client=$enableval
enables_isis_client=$enableval
enables_jura_client=$enableval
enables_saml_client=$enableval
enables_srm_client=$enableval
enables_wsrf_client=$enableval
enables_cache_client=$enableval
enables_unicore_client=$enableval
enables_doc=$enableval
],
[])

AC_ARG_ENABLE(all-data-clients, AC_HELP_STRING([--disable-all-data-clients], [disables all buildable client components providing data handling abilities. Can be overwritten with --enable-* for specific component. It is also possible to use --enable-all-data-clients to overwrite defaults, --enable-all and --enable-all-clients.]),
[
enables_data_client=$enableval
enables_srm_client=$enableval
],
[])

AC_ARG_ENABLE(all-services, AC_HELP_STRING([--disable-all-services], [disables all buildable service componets. Can be overwritten with --enable-* for specific component. It is also possible to use --enable-all-services to overwrite defaults and --enable-all.]),
[
enables_a_rex_service=$enableval
enables_gridftpd_service=$enableval
enables_hopi_service=$enableval
enables_isis_service=$enableval
enables_ldap_service=$enableval
enables_giis_service=$enableval
enables_ldap_monitor=$enableval
enables_janitor_service=$enableval
#enables_charon_service=$enableval
enables_storage_service=$enableval
enables_cache_service=$enableval
],
[])

# Enable/disable switches for third-party.

#  Swig
AC_ARG_ENABLE(swig, AC_HELP_STRING([--disable-swig], [disable all bindings through SWIG]),
[enables_swig=$enableval],[])
if test "$enables_swig" = "yes"; then
  AC_CHECK_PROGS(SWIG, swig)
  if test "x$SWIG" == "x"; then
    enables_swig="no"
  fi
else
  SWIG=""
fi
AM_CONDITIONAL([SWIG_ENABLED],[test "x$SWIG" != "x"])

AC_ARG_ENABLE(hed, AC_HELP_STRING([--disable-hed], [disable building HED libraries and plugins. Do not do that unless You do not want to build anything. Even in that case better use --disable-all.]),
[enables_hed=$enableval],[])

#  Java
dnl Check if Java is explicitly disabled.
if test "$enables_hed" = "yes"; then
JAVAC_FLAGS=
JDK_CFLAGS=
AC_ARG_ENABLE(java, AC_HELP_STRING([--disable-java], [disable the Java binding]),
[enables_java=$enableval],[])
if test "$enables_java" = "yes"; then
  AC_ARG_WITH(jdk, AC_HELP_STRING([--with-jdk=(JDK)], [path to JDK. If unset the system JDK will be used]))

  JPATH=
  if test "x$with_jdk" != "x"; then
    # User specified JDK!
    JPATH=$with_jdk
  else
    # Look for system JDK.
    for i in /usr/lib/jvm/java-*-openjdk* /usr/lib/jvm/java-*-icedtea* /usr/lib/jvm/java-*-gcj* /usr/lib/jvm/java-*-sun* /usr/lib/gcc/*-redhat-linux/*; do
      if test -f $i/include/jni.h; then
        JPATH=$i
        break
      fi
    done
  fi

  if test "x$JPATH" != "x"; then
    JDK_CFLAGS="-I$JPATH/include"

    # Any extra includes? Look for them.
    JAVA_EXTRA_INCLUDE=
    case "${host}" in
      *-pc-mingw32 | *-pc-cygwin)
        JAVA_EXTRA_INCLUDE="win32"
        ;;
      *linux*)
        JAVA_EXTRA_INCLUDE="linux"
        ;;
      *solaris*)
        JAVA_EXTRA_INCLUDE="solaris"
        ;;
    esac
    if test "x$JAVA_EXTRA_INCLUDE" != "x" && test -d $JPATH/include/$JAVA_EXTRA_INCLUDE; then
      JDK_CFLAGS="$JDK_CFLAGS $JDK_CFLAGS/$JAVA_EXTRA_INCLUDE"
    fi

    SAVE_CPPFLAGS=$CPPFLAGS
    CPPFLAGS="$CPPFLAGS $JDK_CFLAGS"
    AC_CHECK_HEADERS(jni.h,[jni_h="yes"],[jni_h="no"])
    CPPFLAGS=$SAVE_CPPFLAGS

    AC_PATH_PROGS(JAVA, java gij, , $JPATH/bin:/usr/bin)
    AC_PATH_PROGS(JAVAC, javac gcj ecj, , $JPATH/bin:/usr/bin)
    AC_PATH_PROGS(JAR, fastjar jar, , $JPATH/bin:/usr/bin)
    break
  fi

  if test "x$with_jdk" = "x" && test "x$JDK_CFLAGS" = "x"; then
    # If JDK was not found in any of the above locations try system locations.
    AC_CHECK_HEADERS(jni.h JavaVM/jni.h,[jni_h="yes"; break],[jni_h="no"])
    AC_CHECK_PROGS(JAVA, java gij)
    AC_CHECK_PROGS(JAVAC, javac gcj ecj)
    AC_CHECK_PROGS(JAR, fastjar jar)
  fi

  if test "x$JAVAC" != "x" && test "x`basename $JAVAC`" = "xgcj"; then
    JAVAC_FLAGS="-C --classpath=."
  elif test "x$JAVAC" != "x" && test "x`basename $JAVAC`" = "xjavac"; then
    JAVAC_FLAGS="-cp ."
  fi

  JAVAWRAP=no
  if test -f java/arc_wrap.cpp || test "x$SWIG" != "x" ; then
    JAVAWRAP=yes
  fi

  if test "x$jni_h" != "xyes"; then
    AC_MSG_NOTICE([Missing Java headers - skipping Java bindings])
    enables_java="no"
  elif test "x$JAVAWRAP" != "xyes"; then
    AC_MSG_NOTICE([Missing pre-compiled Java wrapper or SWIG - skipping Java bindings])
    enables_java="no"
  elif test "x$JAVAC" == "x"; then
    AC_MSG_NOTICE([Missing Java compiler - skipping Java bindings])
    enables_java="no"
  elif test "x$JAR" == "x"; then
    AC_MSG_NOTICE([Missing Java archiver - skipping Java bindings])
    enables_java="no"
  else
    AC_MSG_NOTICE([Java available: $JAVAC])
  fi

fi
fi

AC_MSG_NOTICE([Java enabled: $enables_java])
AM_CONDITIONAL([JAVA_ENABLED],[test "x$enables_java" = "xyes"])

AC_SUBST(JAVAC_FLAGS)
AC_SUBST(JDK_CFLAGS)

# Python
if test "$enables_hed" = "yes"; then
AC_ARG_ENABLE(python, AC_HELP_STRING([--disable-python], [disable the Python binding]),[enables_python=$enableval],[])
if test "$enables_python" = "yes"; then

  AC_ARG_WITH(python, AC_HELP_STRING([--with-python=(PYTHON)], [set the full path to the python program to use]))

  AC_CHECK_PROGS(PYTHON, $with_python python)

  if test "X$PYTHON" != "X"; then
    changequote(<<, >>)dnl
    PYTHON_VERSION=`$PYTHON -c 'import sys; print(sys.version[:3])'`
    PYTHON_VERSION_NODOT=`echo $PYTHON_VERSION | sed 's/\.//g'`

    case $host in
       *-*-mingw*)
          SAVE_PYTHON_VERSION=$PYTHON_VERSION
          PYTHON_VERSION=$PYTHON_VERSION_NODOT
       ;;
    esac

    case $host in
       *-*-mingw*)
    # Use non-mingw32 python here
    PYTHON_CFLAGS=-I`python -c "from distutils import sysconfig; sysconfig.PREFIX = \"${prefix}\" ; print(sysconfig.get_python_inc())"`
    PYTHON_SITE_PACKAGES=`python -c "from distutils import sysconfig; sysconfig.PREFIX = \"${prefix}\" ; print(sysconfig.get_python_lib(1,0,\"${prefix}\"))"`
    PYTHON_LIBS="-L${prefix}/bin -lpython$SAVE_PYTHON_VERSION"
       ;;
                *)
    PYTHON_CFLAGS=-I`$PYTHON -c 'from distutils import sysconfig; print(sysconfig.get_python_inc())'`
    PY_LIBS=`$PYTHON -c "from distutils import sysconfig; print(sysconfig.get_config_vars().get('LIBS'))" | sed s/None//`
    PY_SYSLIBS=`$PYTHON -c "from distutils import sysconfig; print(sysconfig.get_config_vars().get('SYSLIBS'))" | sed s/None//`
    PY_LIBDEST=`$PYTHON -c "from distutils import sysconfig; print(sysconfig.get_config_vars().get('LIBDEST'))" | sed s/None//`
    PYTHON_SITE_PACKAGES=`$PYTHON -c 'from distutils import sysconfig; print(sysconfig.get_python_lib(1,0,"${prefix}"))'`
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

    AC_CHECK_LIB([python$PYTHON_VERSION], [Py_InitializeEx],[
        AC_MSG_NOTICE([Python includes functionality of skipping initialization registration of signal handlers])
        AC_DEFINE(HAVE_PYTHON_INITIALIZE_EX,,[Define if you have Py_InitializeEx function])],[AC_MSG_NOTICE([Python does not include functionality of skipping initialization registration of signal handlers, since its version is below 2.4])])

    LDFLAGS=$SAVE_LDFLAGS
       ;;
    esac
    AC_SUBST(PYTHON_VERSION)
    AC_SUBST(PYTHON_CFLAGS)
    AC_SUBST(PYTHON_LIBS)
    AC_SUBST(PYTHON_SITE_PACKAGES)
  fi

  PYTHONWRAP=no
  if test -f python/arc_wrap.cpp || test "X$SWIG" != "X" ; then
    PYTHONWRAP=yes
  fi

  case $host in
   *-*-mingw*)
     PYTHON_VERSION=$SAVE_PYTHON_VERSION
   ;;
  esac

  if test "X$PYTHONWRAP" != "Xyes"; then
    AC_MSG_NOTICE([Missing pre-compiled Python wrapper and SWIG - skipping Python bindings])
    enables_python=no
  elif test "X$PYTHON" == "X"; then
    AC_MSG_NOTICE([Missing Python - skipping Python bindings])
    enables_python=no
  else
    AC_MSG_NOTICE([Python available: $PYTHON_VERSION])
  fi
fi
fi

AC_MSG_NOTICE([Python enabled: $enables_python])

AM_CONDITIONAL([PYTHON_ENABLED],[test "x$enables_python" = "xyes"])
AM_CONDITIONAL([PYTHON_SERVICE],[test "$PYTHON_VERSION_NODOT" '-ge' "24"])


# using pkgconfig
# check gthread
if test "$enables_hed" = "yes"; then
PKG_CHECK_MODULES(GTHREAD, [gthread-2.0 >= 2.4.7])
AC_SUBST(GTHREAD_CFLAGS)
AC_SUBST(GTHREAD_LIBS)
fi

# check glibmm
# check for giomm which became a part of glibmm as of version 2.16
if test "$enables_hed" = "yes"; then
"$PKG_CONFIG" giomm-2.4
if test "$?" = '1'; then
  PKG_CHECK_MODULES(GLIBMM, [glibmm-2.4 >= 2.4.7])
else
  PKG_CHECK_MODULES(GLIBMM, [giomm-2.4])
  AC_DEFINE(HAVE_GIOMM,, [define if giomm is supported in glibmm])
fi
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
  AC_TRY_COMPILE([#include <glibmm/optioncontext.h>],
    [Glib::OptionContext ctx; ctx.get_help();],[
      AC_DEFINE(HAVE_GLIBMM_OPTIONCONTEXT_GET_HELP,,
        [define if glibmm has Glib::OptionContext::get_help()])
    ], [
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
fi

# check libxml
if test "$enables_hed" = "yes"; then
PKG_CHECK_MODULES(LIBXML2, [libxml-2.0 >= 2.4.0])
AC_SUBST(LIBXML2_CFLAGS)
AC_SUBST(LIBXML2_LIBS)
fi

# check openssl
if test "$enables_hed" = "yes"; then
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
fi

# check cppunit
if test "$enables_hed" = "yes"; then
AC_ARG_ENABLE(cppunit, AC_HELP_STRING([--disable-cppunit], [disable cppunit-based UNIT testing of code]),[enables_cppunit=$enableval],[])
if test "$enables_cppunit" = "yes"; then
  PKG_CHECK_MODULES(CPPUNIT, [cppunit],[],
    [AC_PATH_PROG(CPPUNIT_CONFIG, cppunit-config, no)
    if test "x$CPPUNIT_CONFIG" = "xno"; then
        AC_MSG_WARN([cppunit-config not found - no UNIT testing will be performed])
        CPPUNIT_CFLAGS=
        CPPUNIT_LIBS=
        enables_cppunit="no"
    else
        CPPUNIT_CFLAGS="`$CPPUNIT_CONFIG --cflags`"
        CPPUNIT_LIBS="`$CPPUNIT_CONFIG --libs`"
    fi])
  if test "x$CPPUNIT_CONFIG" != "xno" || test "x$CPPUNIT_PKG_ERRORS" != "x"
  then
    TEST_DIR=test
  else
    enables_cppunit=no
    TEST_DIR=
  fi
fi
AC_SUBST(CPPUNIT_CFLAGS)
AC_SUBST(CPPUNIT_LIBS)
AC_SUBST(TEST_DIR)
else
  enables_cppunit="no"
fi

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
MACOS=""
case "${host}" in
  *darwin*)
    MACOS="yes"
    ;;
esac

if test "x$MACOS" = "xyes"; then
  AC_DEFINE(_MACOSX,,[Define if compiling for MacOSX])
fi

if test "$enables_hed" = "yes"; then
XMLSEC_MIN_VERSION="1.2.4"
XMLSEC_OPENSSL_MIN_VERSION="1.2.4"
XMLSEC_CONFIG="xmlsec1-config"
XMLSEC_CFLAGS=""
XMLSEC_LIBS=""
XMLSEC_INSTALLED=no
dnl Check if xmlsec1 is explicitly disabled, default is enable.
AC_ARG_ENABLE(xmlsec1, AC_HELP_STRING([--disable-xmlsec1], [disable features which need xmlsec1 library]),[enables_xmlsec1=$enableval],[])
if test "x$enables_xmlsec1" = "xyes"; then
  AC_ARG_WITH(xmlsec1, [  --with-xmlsec1=(PATH)  xmlsec1 location])
  if test "x$with_xmlsec1" = "x" ; then
    PKG_CHECK_MODULES(XMLSEC, [xmlsec1 >= $XMLSEC_MIN_VERSION],
        [XMLSEC_INSTALLED=yes], [XMLSEC_INSTALLED=no])
    if test "x$XMLSEC_INSTALLED" = "xyes" ; then
      PKG_CHECK_MODULES(XMLSEC_OPENSSL, [xmlsec1-openssl >= $XMLSEC_OPENSSL_MIN_VERSION],
          [XMLSEC_INSTALLED=yes],[XMLSEC_INSTALLED=no])
    fi
  fi

  if test "x$XMLSEC_INSTALLED" = "xno" -a "x$MACOS" != "xyes"; then
      AC_MSG_CHECKING(for xmlsec1 libraries >= $XMLSEC_MIN_VERSION)
      if test "x$with_xmlsec1" != "x" ; then
        XMLSEC_CONFIG=$with_xmlsec1/bin/$XMLSEC_CONFIG
      fi
      "$XMLSEC_CONFIG" --version 2>/dev/null 1>/dev/null
      if test "$?" != '0' ; then
        AC_MSG_WARN(Could not find xmlsec1 anywhere; The xml security related functionality will not be compiled)
      else
        vers=`$XMLSEC_CONFIG --version 2>/dev/null | awk -F. '{ printf "%d", ($1 * 1000 + $2) * 1000 + $3;}'`
        minvers=`echo $XMLSEC_MIN_VERSION | awk -F. '{ printf "%d", ($1 * 1000 + $2) * 1000 + $3;}'`
        if test "$vers" -ge "$minvers" ; then
          XMLSEC_LIBS="`$XMLSEC_CONFIG --libs`"
          XMLSEC_CFLAGS="`$XMLSEC_CONFIG --cflags`"
          #check the xmlsec1-openssl here
          PKG_CONFIG_PATH="$with_xmlsec1/lib/pkgconfig/:$PKG_CONFIG_PATH"
          PKG_CHECK_MODULES(XMLSEC_OPENSSL, [xmlsec1-openssl >= $XMLSEC_OPENSSL_MIN_VERSION],
              [XMLSEC_INSTALLED=yes],[XMLSEC_INSTALLED=no])
        else
          AC_MSG_WARN(You need at least xmlsec1 $XMLSEC_MIN_VERSION for this version of arc1)
        fi
      fi
  elif test "x$XMLSEC_INSTALLED" = "xno" -a "x$MACOS" = "xyes"; then
    #MACOS has no "ldd" which is needed by xmlsec1-config, so here simply we use PKG_CHECK_MODULES
    PKG_CONFIG_PATH="$with_xmlsec1/lib/pkgconfig/:$PKG_CONFIG_PATH"
    PKG_CHECK_MODULES(XMLSEC, [xmlsec1 >= $XMLSEC_MIN_VERSION],
        [XMLSEC_INSTALLED=yes], [XMLSEC_INSTALLED=no])
    if test "x$XMLSEC_INSTALLED" = "xyes" ; then
      PKG_CHECK_MODULES(XMLSEC_OPENSSL, [xmlsec1-openssl >= $XMLSEC_OPENSSL_MIN_VERSION],
          [XMLSEC_INSTALLED=yes],[XMLSEC_INSTALLED=no])
    fi
  fi

  AC_SUBST(XMLSEC_CFLAGS)
  AC_SUBST(XMLSEC_LIBS)
  AC_SUBST(XMLSEC_OPENSSL_CFLAGS)
  AC_SUBST(XMLSEC_OPENSSL_LIBS)

  #AC_SUBST(XMLSEC_CONFIG)
  #AC_SUBST(XMLSEC_MIN_VERSION)
  enables_xmlsec1="$XMLSEC_INSTALLED"
fi
else
  enables_xmlsec1="no"
fi

#########################
#
# Check libmysqlclient
#
#########################
MYSQL_INSTALLED=no
if test "$enables_hed" = "yes"; then
MYSQL_CONFIG="mysql_config"
MYSQL_CFLAGS=""
MYSQL_LIBS=""
dnl Check if libmysqlclient is explicitly enabled, default is disable.
AC_ARG_ENABLE(mysql, AC_HELP_STRING([--enable-mysql], [enable use of the MySQL client library]),[enables_mysql="$enableval"],[])
# Ask user for path to libmysqlclient
if test "x$enables_mysql" = "xyes"; then
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
  enables_mysql=$MYSQL_INSTALLED
fi
AC_MSG_NOTICE([MySQL client library enabled: $MYSQL_INSTALLED])
fi
AM_CONDITIONAL([MYSQL_LIBRARY_ENABLED],[test "x$MYSQL_INSTALLED" = "xyes"])

# Check ldap-monitor
AC_ARG_ENABLE(ldap_monitor, AC_HELP_STRING([--enable-ldap-monitor], [enable use of the ldap monitor]),[enables_ldap_monitor="$enableval"],[])
if test "x$enables_ldap_monitor" = "xyes"; then
  AC_ARG_WITH(ldap_monitor, [  --with-ldap-monitor=(PATH) where to install the monitor, eg /var/www/ldap-monitor or /usr/share/arc/ldap-monitor])
  AC_MSG_CHECKING(for ldap-monitor installation path)
  if test "x$with_ldap_monitor" != "x" ; then
      ldap_monitor_prefix=$with_ldap_monitor
  else
      ldap_monitor_prefix=${datadir}/arc/ldap-monitor
  fi
  AC_MSG_RESULT([$ldap_monitor_prefix])
  AC_SUBST(ldap_monitor_prefix)
fi

# Check ws-monitor
AC_ARG_ENABLE(ws_monitor, AC_HELP_STRING([--enable-ws-monitor], [enable use of the ws monitor]),[enables_ws_monitor="$enableval"],[])
if test "x$enables_ws_monitor" = "xyes"; then
  AC_ARG_WITH(ws_monitor, [  --with-ws-monitor=(PATH) where to install the monitor, eg /var/www/ws-monitor or /usr/share/arc/ws-monitor])
  AC_MSG_CHECKING(for ws-monitor installation path)
  if test "x$with_ws_monitor" != "x" ; then
      ws_monitor_prefix=$with_ws_monitor
  else
      ws_monitor_prefix=${datadir}/arc/ws-monitor
  fi
  AC_MSG_RESULT([$ws_monitor_prefix])
  AC_SUBST(ws_monitor_prefix)
fi

# check zlib
ZLIB_CFLAGS=
ZLIB_LDFLAGS=
ZLIB_LIBS=
if test "$enables_hed" = "yes"; then
SAVE_CPPFLAGS=$CPPFLAGS
SAVE_LDFLAGS=$LDFLAGS
AC_ARG_WITH(zlib,
  AC_HELP_STRING([--with-zlib=PATH], [where zlib is installed]),
    [ if test -d "$withval"; then
        ZLIB_CFLAGS="${CPPFLAGS} -I$withval/include"
        ZLIB_LDFLAGS="${LDFLAGS} -L$withval/lib"
      fi
    ]
)
CPPFLAGS="$CPPFLAGS $ZLIB_CFLAGS"
LDFLAGS="$LDFLAGS $ZLIB_LDFLAGS"
AC_CHECK_HEADER([zlib.h],[ZLIB_CFLAGS="$ZLIB_CFLAGS"],AC_MSG_ERROR([unable to find zlib header files]))
AC_CHECK_LIB([z],[deflateInit2_],[ZLIB_LIBS="$ZLIB_LDFLAGS -lz"],AC_MSG_ERROR([unable to link with zlib library]))
CPPFLAGS=$SAVE_CPPFLAGS
LDFLAGS=$SAVE_LDFLAGS
fi
AC_SUBST(ZLIB_CFLAGS)
AC_SUBST(ZLIB_LIBS)

# check gridsite
AC_ARG_WITH(gridsite,
  AC_HELP_STRING([--with-gridsite=PATH], [where gridsite is installed]),
  [
    if test -d "$withval"; then
        CPPFLAGS="${CPPFLAGS} -I$withval/include"
        LDFLAGS="${LDFLAGS} -L$withval/lib"
    fi
  ]
)
SAVE_LIBS=$LIBS
if test "x$enables_gacl" = "xyes"; then
  AC_CHECK_HEADER([gridsite.h],, enables_gacl="no")
fi
if test "x$enables_gacl" = "xyes"; then
  AC_CHECK_LIB([gridsite], [GRSTgaclInit], [AC_MSG_NOTICE([passed])], enables_gacl="no")
fi
LIBS=$SAVE_LIBS
AM_CONDITIONAL(GACL_ENABLED, test "x$enables_gacl" = "xyes")
if test "x$enables_gacl" = "xyes"; then
   AC_DEFINE(HAVE_GACL,, [define if gridsite GACL is available])
fi

# check ARGUS
ARGUS_CFLAGS=
ARGUS_LIBS=
AC_ARG_ENABLE(argus, AC_HELP_STRING([--enable-argus], [enable use of Argus PEP V2 libraries]),[enables_argus="$enableval"],[])
if test "x$enables_argus" = "xyes"; then
  AC_ARG_WITH(argus,
    AC_HELP_STRING([--with-argus=PATH], [ARGUS PEP installation path]),
    [
      PKG_CONFIG_PATH="$withval/lib/pkgconfig:$PKG_CONFIG_PATH"
    ]
  )
  PKG_CHECK_MODULES(ARGUS, [libargus-pep >= 2.0.0], [], [
    AC_MSG_NOTICE([Failed to find Argus PEP libraries with version >= 2])
    argus_enables=no
  ])
fi
AC_SUBST(ARGUS_CFLAGS)
AC_SUBST(ARGUS_LIBS)
AM_CONDITIONAL(ARGUS_ENABLED, test "x$enables_argus" = "xyes")

###############################################
#
# Check for Berkeley DB C++
#
###############################################
DBCXX_LIBS=""
DBCXX_CPPFLAGS=

if test "$enables_hed" = "yes"; then

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

if test "x$dbcxx_include_paths" = "x"
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
  SAVE_LDFLAGS=$LDFLAGS
  SAVE_CXXFLAGS=$CXXFLAGS

  case $host in
    *-*-mingw*)
      CXXFLAGS="-I$dbcxx_dir $CXXFLAGS"
      ;;
    *)
      # pthread needed for RH9
      LDFLAGS="$LDFLAGS -lpthread"
      ;;
  esac

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
    AC_MSG_WARN([BerkeleyDB library libdb_cxx was not found!])
    AC_CHECK_LIB(db_cxx-4.7,main,DBCXX_LIBS="-ldb_cxx-4.7",DBCXX_LIBS="")
  fi

  if test "$DBCXX_LIBS" = ""
  then
    AC_MSG_WARN([BerkeleyDB library libdb_cxx-4.7 was not found!])
  fi

  if test "$DBCXX_LIBS" = ""
  then
    AC_MSG_WARN([No BerkeleyDB library found!])
  fi

  LDFLAGS=$SAVE_LDFLAGS
  CXXFLAGS=$SAVE_CXXFLAGS
fi

AC_SUBST(DBCXX_LIBS)
if test ! "x$DBCXX_LIBS" = "x"
then
  # Mingw need -I$dbcxx_dir

  SAVE_CXXFLAGS=$CXXFLAGS
  CXXFLAGS="-I$dbcxx_dir $CXXFLAGS"
  AC_DBCXX_HAVE_DBDEADLOCKEXCEPTION
  CXXFLAGS=$SAVE_CXXFLAGS
fi
AC_LANG_RESTORE

fi
AM_CONDITIONAL([DBXML_ENABLED],[test ! "x$DBCXX_LIBS" = "x"])

# globus/gpt packages
if test "$enables_hed" = "yes"; then
PKG_CHECK_MODULES(GLOBUS_COMMON, [globus-common], [
GLOBUS_COMMON_VERSION=`$PKG_CONFIG --modversion globus-common`], [
GPT_PKG(globus_common)
])
AC_SUBST(GLOBUS_COMMON_CFLAGS)
AC_SUBST(GLOBUS_COMMON_LIBS)

PKG_CHECK_MODULES(GLOBUS_GSSAPI_GSI, [globus-gssapi-gsi], [
GLOBUS_GSSAPI_GSI_VERSION=`$PKG_CONFIG --modversion globus-gssapi-gsi`], [
GPT_PKG(globus_gssapi_gsi)
])
AC_SUBST(GLOBUS_GSSAPI_GSI_CFLAGS)
AC_SUBST(GLOBUS_GSSAPI_GSI_LIBS)

PKG_CHECK_MODULES(GLOBUS_RLS_CLIENT, [globus-rls-client], [
GLOBUS_RLS_CLIENT_VERSION=`$PKG_CONFIG --modversion globus-rls-client`], [
GPT_PKG(globus_rls_client)
])
AC_SUBST(GLOBUS_RLS_CLIENT_CFLAGS)
AC_SUBST(GLOBUS_RLS_CLIENT_LIBS)

PKG_CHECK_MODULES(GLOBUS_FTP_CLIENT, [globus-ftp-client], [
GLOBUS_FTP_CLIENT_VERSION=`$PKG_CONFIG --modversion globus-ftp-client`], [
GPT_PKG(globus_ftp_client)
])
AC_SUBST(GLOBUS_FTP_CLIENT_CFLAGS)
AC_SUBST(GLOBUS_FTP_CLIENT_LIBS)

PKG_CHECK_MODULES(GLOBUS_FTP_CONTROL, [globus-ftp-control], [
GLOBUS_FTP_CONTROL_VERSION=`$PKG_CONFIG --modversion globus-ftp-control`], [
GPT_PKG(globus_ftp_control)
])
AC_SUBST(GLOBUS_FTP_CONTROL_CFLAGS)
AC_SUBST(GLOBUS_FTP_CONTROL_LIBS)

PKG_CHECK_MODULES(GLOBUS_IO, [globus-io], [
GLOBUS_IO_VERSION=`$PKG_CONFIG --modversion globus-io`], [
GPT_PKG(globus_io)
])
AC_SUBST(GLOBUS_IO_CFLAGS)
AC_SUBST(GLOBUS_IO_LIBS)

# Check for new globus thread model selection
SAVE_CFLAGS=$CFLAGS
SAVE_LIBS=$LIBS
CFLAGS="$CFLAGS $GLOBUS_COMMON_CFLAGS"
LIBS="$LIBS $GLOBUS_COMMON_LIBS"
AC_CHECK_FUNCS(globus_thread_set_model)
CFLAGS=$SAVE_CFLAGS
LIBS=$SAVE_LIBS

# Check for gridftp-v2
SAVE_CFLAGS=$CFLAGS
SAVE_LIBS=$LIBS
CFLAGS="$CFLAGS $GLOBUS_FTP_CLIENT_CFLAGS"
LIBS="$LIBS $GLOBUS_FTP_CLIENT_LIBS"
AC_CHECK_FUNCS(globus_ftp_client_handleattr_set_gridftp2)
CFLAGS=$SAVE_CFLAGS
LIBS=$SAVE_LIBS

globus_openssl_detected=
PKG_CHECK_MODULES(GLOBUS_OPENSSL, [globus-openssl], [
GLOBUS_OPENSSL_VERSION=`$PKG_CONFIG --modversion globus-openssl`], [
GPT_PKG(globus_openssl)
])
if test ! "x$GLOBUS_OPENSSL_LIBS" = "x" ; then
  globus_openssl_detected=`echo "$GLOBUS_OPENSSL_LIBS" | grep "lssl_$GPT_FLAVOR"`
  if test ! "x$globus_openssl_detected" = "x" ; then
    globus_openssl_detected="yes"
  fi
fi
if test "x$globus_openssl_detected" = "xyes" ; then
AC_MSG_RESULT([
Globus own OpenSSL library detected.
In order to avoid runtime conflicts following components will be disabled:
RLS DMC,
GridFTP DMC,
SRM DMC,
GSI MCC.
To enable these components use Globus compiled for system OpenSSL.
])
GLOBUS_RLS_CLIENT_VERSION=
GLOBUS_FTP_CLIENT_VERSION=
GLOBUS_FTP_CONTROL_VERSION=
GLOBUS_IO_VERSION=
GLOBUS_GSSAPI_GSI_VERSION=
fi

if test "x$GLOBUS_IO_VERSION" = "x"; then
  IO_VERSION_MAJOR=0
else
  IO_VERSION_MAJOR=`echo "$GLOBUS_IO_VERSION" | sed 's/^\([[^.]]\).*/\1/'`;
fi
AC_DEFINE_UNQUOTED(GLOBUS_IO_VERSION,$IO_VERSION_MAJOR,[Globus IO version])

dnl
dnl DEFAULT_GLOBUS_LOCATION
dnl
AC_MSG_CHECKING(for DEFAULT_GLOBUS_LOCATION)
# GLOBUS_LOCATION is set by GPT macros
DEFAULT_GLOBUS_LOCATION="$GLOBUS_LOCATION"
AC_MSG_RESULT($DEFAULT_GLOBUS_LOCATION)
AC_SUBST(DEFAULT_GLOBUS_LOCATION)

# Check for LFC
AC_ARG_ENABLE(lfc, AC_HELP_STRING([--disable-lfc], [disable the LFC support, default is enable]),[enables_lfc="$enableval"],[])
if test "x$enables_lfc" = "xyes"; then
  LFC_CFLAGS="-I/usr/include/lfc"
  LFC_LDFLAGS=""
  AC_ARG_WITH(lfc, [  --with-lfc=(PATH)  LFC location])
  if test ! "x$with_lfc" = "x" ; then
    LFC_LOCATION="$with_lfc"
    LFC_CFLAGS="-I$LFC_LOCATION/include/lfc"
    LFC_LDFLAGS="-L$LFC_LOCATION/lib"
  fi
  SAVE_CPPFLAGS=$CPPFLAGS
  CPPFLAGS="$CPPFLAGS $LFC_CFLAGS"
  AC_CHECK_HEADER(lfc_api.h, [], [enables_lfc="no"])
  CPPFLAGS=$SAVE_CPPFLAGS
  SAVE_LDFLAGS=$LDFLAGS
  LDFLAGS="$LDFLAGS $LFC_LDFLAGS"
  AC_CHECK_LIB([lfc], [lfc_apiinit],
               [LFC_LIBS="$LFC_LDFLAGS -llfc"], [enables_lfc="no"])
  # Check if lcgdm is a separate library
  AC_CHECK_LIB([lcgdm], [C__serrno], [LFC_LIBS="$LFC_LIBS -llcgdm"],
  AC_CHECK_LIB([lfc], [C__serrno], [LFC_LIBS="$LFC_LIBS"], [enables_lfc="no"]))
  LDFLAGS=$SAVE_LDFLAGS
fi
AC_SUBST(LFC_CFLAGS)
AC_SUBST(LFC_LIBS)
fi

# Setup conditionals
AM_CONDITIONAL([GLOBUSUTILS_ENABLED], test -n "$GLOBUS_COMMON_VERSION")
AM_CONDITIONAL([RLS_ENABLED], test -n "$GLOBUS_RLS_CLIENT_VERSION")
AM_CONDITIONAL([GRIDFTP_ENABLED], test -n "$GLOBUS_FTP_CLIENT_VERSION")
AM_CONDITIONAL([LFC_ENABLED], test x$enables_lfc = xyes)
AM_CONDITIONAL([OPENSSL_PROXY_ENABLED], test x$OPENSSL_PROXY = xyes)
AM_CONDITIONAL([OPENSSL_X509_VERIFY_PARAM_ENABLED], test x$OPENSSL_X509_VERIFY_PARAM = xyes)
AM_CONDITIONAL([XMLSEC_ENABLED], test x$XMLSEC_INSTALLED = xyes)
AM_CONDITIONAL([CPPUNIT_ENABLED], test x$enables_cppunit = xyes)

enables_srm_dmc=no
if test "$enables_hed" = "yes"; then
  enables_srm_dmc=yes
fi

if test "$enables_giis_service" = "yes"; then
  if test "$enables_ldap_service" = "no"; then
    enables_ldap_service="yes"
  fi
fi

AM_CONDITIONAL([SRM_DMC_ENABLED],[test "x$enables_srm_dmc" = "xyes"])

enables_gsi_mcc=no
if test "$enables_hed" = "yes"; then
if test ! "x$GLOBUS_GSSAPI_GSI_VERSION" = 'x'; then
  enables_gsi_mcc=yes
fi
fi
AM_CONDITIONAL([GSI_MCC_ENABLED],[test "x$enables_gsi_mcc" = "xyes"])

# Setup defines
if test -n "$GLOBUS_RLS_CLIENT_VERSION"; then
    AC_DEFINE(HAVE_GLOBUS_RLS_CLIENT,,[define if GLOBUS RLS client package is available])
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

# Setup messages for reporting
enables_rls=no
if test -n "$GLOBUS_RLS_CLIENT_VERSION" ; then enables_rls=yes; fi
enables_gridftp=no
if test -n "$GLOBUS_FTP_CLIENT_VERSION" ; then enables_gridftp=yes; fi

# Check for LDAP

if test "$enables_hed" = "yes"; then
LDAP=no
AC_ARG_ENABLE(ldap, AC_HELP_STRING([--disable-ldap], [disable the LDAP support - requires OpenLDAP]),[enables_ldap="$enableval"],[])
if test "x$enables_ldap" = "xyes"; then
  AC_CHECK_HEADER(ldap.h, [
    LDAP=yes

    SAVE_LDFLAGS=$LDFLAGS

    case "${host}" in
      *-*-mingw32):
      ;;
      *):
        LDFLAGS=-lpthread
      ;;
    esac
    AC_CHECK_LIB([ldap_r], [ldap_first_message], [
      AC_CHECK_LIB([ldap_r], [ldap_initialize], [
        AC_DEFINE(HAVE_LDAP_INITIALIZE,[],[Define if you have ldap_initialize function])
      ])
      LDAP_LIBS=-lldap_r ], [
      AC_CHECK_LIB([ldap], [ldap_first_message], [
        AC_CHECK_LIB([ldap], [ldap_initialize], [ AC_DEFINE(HAVE_LDAP_INITIALIZE,[],[Define if you have ldap_initialize function])
      ])
      LDAP_LIBS=-lldap ], [
      LDAP=no
    ])
  ])
  AC_CHECK_LIB([lber], [ber_init], [LDAP_LIBS="$LDAP_LIBS -llber"], [])
  AC_SUBST(LDAP_LIBS)
  LDFLAGS=$SAVE_LDFLAGS
  ],

  [
    LDAP=no

    # Try native LDAP on Win32 if OpenLDAP fails
    case "${host}" in
      *-*-mingw32)
    AC_CHECK_HEADER([winldap.h],
    [
      AC_CHECK_LIB([wldap32], [ldap_init],
      [
        LDAP=yes
        LDAP_LIBS="-lwldap32"
        AC_DEFINE(USE_WIN32_LDAP_API,,[Define if using WIN32 LDAP API])
      ])
    ], [], [[#include <windows.h>]]) ;;
    esac

  ])

  enables_ldap="$LDAP"
fi
else
  enables_ldap="no"
fi
AM_CONDITIONAL([LDAP_ENABLED], test x$LDAP = xyes)
if test "x$LDAP" = "xyes"; then
  AC_DEFINE(HAVE_LDAP,[],[Define if OpenLDAP is available])
fi

# Check for the uuid lib
UUID_LIBS=""
if test "$enables_hed" = "yes"; then
AC_CHECK_HEADER(uuid/uuid.h, [
  AC_CHECK_FUNC([uuid_generate], [UUID_LIBS=], [
    AC_CHECK_LIB([uuid], [uuid_generate], [UUID_LIBS=-luuid], [
      AC_MSG_NOTICE([Can't find library containing uuid implementation])
    ])
  ])
], [AC_MSG_NOTICE([Can't find uuid header])])
AC_SUBST(UUID_LIBS)
LIBS="$LIBS $UUID_LIBS"
fi

# Check for dlopen
DLOPEN_LIBS=""
if test "$enables_hed" = "yes"; then
AC_CHECK_FUNC([dlopen], [DLOPEN_LIBS=], [
  AC_CHECK_LIB([dl], [dlopen], [DLOPEN_LIBS=-ldl], [
    AC_MSG_NOTICE([Can't find library containing dlopen implementation])
  ])
])
AC_SUBST(DLOPEN_LIBS)
fi

# check for fsusage
if test "$enables_hed" = "yes"; then
gl_FSUSAGE
fi


if test "$enables_hed" = "yes"; then

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
AC_FUNC_MEMCMP
AC_FUNC_MKTIME
# GNU compatible *ALLOC functions are available on Windows
# The test will however fail when cross-compiling with mingw
case "${host}" in
  *-*-mingw32) : ;;
            *)
AC_FUNC_MALLOC
AC_FUNC_REALLOC
;;
esac
AC_FUNC_SELECT_ARGTYPES
AC_TYPE_SIGNAL
AC_FUNC_STRERROR_R
AC_FUNC_STAT
AC_CHECK_FUNCS([acl dup2 floor ftruncate gethostname getdomainname getpid gmtime_r lchown localtime_r memchr memmove memset mkdir mkfifo regcomp rmdir select setenv socket strcasecmp strchr strcspn strdup strerror strncasecmp strstr strtol strtoul strtoull timegm tzset unsetenv getopt_long_only getgrouplist mkdtemp])
AC_CHECK_LIB([resolv], [res_query], [LIBRESOLV=-lresolv], [LIBRESOLV=])
AC_CHECK_LIB([resolv], [__dn_skipname], [LIBRESOLV=-lresolv], [LIBRESOLV=])
AC_CHECK_LIB([nsl], [gethostbyname], [LIBRESOLV="$LIBRESOLV -lnsl"], [])
AC_CHECK_LIB([nsl], [getdomainname])
AC_SUBST(LIBRESOLV)

fi

# check for platfom specific extra libraries and flags

EXTRA_LIBS=""
REGEX_LIBS=""
SOCKET_LIBS=""
WIN32=""
case "${host}" in
  *-*-mingw32)
    WIN32="yes"
    REGEX_LIBS="-lregex"
    SOCKET_LIBS="-lws2_32"
    EXTRA_LIBS="-lole32"
    # its required to libtool generate .dlls on win32 using mingw
    LDFLAGS="$LDFLAGS -no-undefined"
    AC_DEFINE(HAVE_GETDOMAINNAME,,[windows has gethostname method])
    AC_CHECK_PROGS(WINDRES, windres.exe i686-pc-mingw32-windres)
    ;;
  *solaris*)
    SOCKET_LIBS="-lsocket"
    CFLAGS="$CFLAGS -D_POSIX_PTHREAD_SEMANTICS"
    CXXFLAGS="$CXXFLAGS -D_POSIX_PTHREAD_SEMANTICS"
    ;;
  *)
    AC_DEFINE(HAVE_GETDOMAINNAME,,[let's suppose the unix-alike (except solaris) has gethostname method])
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

AC_CHECK_PROGS(PDFLATEX, pdflatex)
AC_CHECK_PROGS(DOXYGEN, doxygen)
AC_CHECK_PROGS(DOT, dot)

# Check if user asks to skip documentation build
AC_ARG_ENABLE(doc, AC_HELP_STRING([--disable-doc], [disable building documentation (requires doxygen and pdflatex)]),[enables_doc=$enableval],[])
#if test "x$enables_doc" = "xyes"; then
#  There is no point disabling docs due to missing tools since the pdf
#  files are both in svn and in the dist tarball
#  if test "x$PDFLATEX" = "x"; then
#    enables_doc="no"
#    AC_MSG_NOTICE([WARNING: Missing pdflatex - documentation won't be built])
#  elif test "x$DOXYGEN" = "x"; then
#    enables_doc="no"
#    AC_MSG_NOTICE([WARNING: Missing doxygen - documentation won't be built])
#  elif test "x$DOT" = "x"; then
#    enables_doc="no"
#    AC_MSG_NOTICE([WARNING: Missing dot - documentation won't be built])
#  fi
#fi
AC_MSG_NOTICE([Documentation enabled: $enables_doc])
AM_CONDITIONAL([DOC_ENABLED],[test "x$enables_doc" = "xyes"])

AM_CONDITIONAL([PYDOXYGEN],[test -f python/pydoxygen.i -o "x$DOXYGEN" != "x"])

# Check for explicitely and implicitely disabled services
if test "x$WIN32" = "xyes" ; then
  AC_MSG_NOTICE([In WIN32 environment many (all) services are not supported yet])
fi
# A-Rex
AC_ARG_ENABLE(a_rex_service, AC_HELP_STRING([--disable-a-rex-service], [disable building A-Rex service]),
  [enables_a_rex_service=$enableval],[])
if test "$enables_a_rex_service" = "yes"; then
  if test "x$WIN32" = "xyes" ; then
    AC_MSG_NOTICE([A-Rex can't be built for WIN32 environment - disabling])
    enables_a_rex_service="no"
  fi
fi
AC_MSG_NOTICE([A-Rex service enabled: $enables_a_rex_service])
AM_CONDITIONAL([A_REX_SERVICE_ENABLED],[test "x$enables_a_rex_service" = "xyes"])

# Gridftpd
AC_ARG_ENABLE(gridftpd_service, AC_HELP_STRING([--disable-gridftpd-service], [disable building Gridftpd service]),
  [enables_gridftpd_service=$enableval],[])
if test "$enables_gridftpd_service" = "yes"; then
  if test "x$WIN32" = "xyes" ; then
    AC_MSG_NOTICE([Gridftpd can't be built for WIN32 environment - disabling])
    enables_gridftpd_service="no"
  elif test -z "$GLOBUS_COMMON_VERSION" ; then
    AC_MSG_NOTICE([Gridftpd can't be built without Globus - disabling])
    enables_gridftpd_service="no"
  fi

  #check for struct statfs
  AC_CHECK_FUNCS([fstatfs])
  AC_CHECK_HEADERS([sys/param.h sys/statfs.h sys/mount.h sys/vfs.h])dnl
  AC_CHECK_MEMBERS([struct statfs.f_type],,,
    [$ac_includes_default
     #if HAVE_SYS_STATFS_H
     #include <sys/statfs.h>
     #endif
     #if HAVE_SYS_MOUNT_H
     #include <sys/mount.h>
     #endif
     #if HAVE_SYS_VFS_H
     #include <sys/vfs.h>
     #endif])
fi
AC_MSG_NOTICE([Gridftpd service enabled: $enables_gridftpd_service])
AM_CONDITIONAL([GRIDFTPD_SERVICE_ENABLED],[test "x$enables_gridftpd_service" = "xyes"])

# ISIS
AC_ARG_ENABLE(isis_service, AC_HELP_STRING([--disable-isis-service], [disable building ISIS Service]),
  [enables_isis_service=$enableval],[])
if test "$enables_isis_service" = "yes"; then
  if test "x$WIN32" = "xyes" ; then
    AC_MSG_NOTICE([ISIS can't be built for WIN32 environment - disabling])
    enables_isis_service="no"
  fi
fi
if test "$enables_isis_service" = "yes"; then
  if test "$DBCXX_LIBS" = ""; then
    enables_isis_service="no"
    AC_MSG_NOTICE([WARNING: Missing db_cxx - disabling ISIS])
  fi
fi
AC_MSG_NOTICE([ISIS service enabled: $enables_isis_service])
AM_CONDITIONAL([ISIS_SERVICE_ENABLED],[test "x$enables_isis_service" = "xyes"])

# LDAP service
AC_ARG_ENABLE(ldap_service, AC_HELP_STRING([--disable-ldap-service], [disable building LDAP Infosystem Service]),
  [enables_ldap_service=$enableval],[])
if test "$enables_ldap_service" = "yes"; then
  if test "x$WIN32" = "xyes" ; then
    AC_MSG_NOTICE([LDAP infosystem can't be built for WIN32 environment - disabling])
    enables_ldap_service="no"
  fi
fi

AC_MSG_NOTICE([LDAP Infosystem service enabled: $enables_ldap_service])
AM_CONDITIONAL([LDAP_SERVICE_ENABLED],[test "x$enables_ldap_service" = "xyes"])

# GIIS service
AC_ARG_ENABLE(giis_service, AC_HELP_STRING([--disable-giis-service], [disable building GIIS Service]),
  [enables_giis_service=$enableval],[])
if test "$enables_giis_service" = "yes"; then
  if test "x$WIN32" = "xyes" ; then
    AC_MSG_NOTICE([GIIS can't be built for WIN32 environment - disabling])
    enables_giis_service="no"
  fi
  AC_CHECK_HEADERS(lber.h, [],[enables_giis_service=no])
  AC_CHECK_HEADERS(ldap_features.h, [],[enables_giis_service=no])
fi
if test "$enables_ldap_service" = "no"; then
  if test "$enables_giis_service" = "yes"; then
    enables_giis_service="no"
    AC_MSG_NOTICE([WARNING: GIIS service can't be enabled without LDAP Infosys, disabling GIIS])
  fi
fi
AC_MSG_NOTICE([GIIS service enabled: $enables_giis_service])
AM_CONDITIONAL([GIIS_SERVICE_ENABLED],[test "x$enables_giis_service" = "xyes"])

# LDAP monitor
AC_ARG_ENABLE(ldap_monitor, AC_HELP_STRING([--disable-ldap-monitor], [disable building LDAP Monitor]),
  [enables_ldap_monitor=$enableval],[])

AC_MSG_NOTICE([LDAP Monitor enabled: $enables_ldap_monitor])
AM_CONDITIONAL([LDAP_MONITOR_ENABLED],[test "x$enables_ldap_monitor" = "xyes"])

# WS monitor
AC_ARG_ENABLE(ws_monitor, AC_HELP_STRING([--disable-ws-monitor], [disable building WS Monitor]),
  [enables_ws_monitor=$enableval],[])

AC_MSG_NOTICE([WS Monitor enabled: $enables_ws_monitor])
AM_CONDITIONAL([WS_MONITOR_ENABLED],[test "x$enables_ws_monitor" = "xyes"])

## Charon service
#AC_ARG_ENABLE(charon_service, AC_HELP_STRING([--disable-charon-service], [disable building Charon Service]),
#  [enables_charon_service=$enableval],[])
#if test "$enables_charon_service" = "yes"; then
#  if test "x$WIN32" = "xyes" ; then
#    enables_charon_service="no"
#    AC_MSG_NOTICE([Charon can't be built for WIN32 environment - disabling])
#  fi
#fi
#AC_MSG_NOTICE([Charon Service enabled: $enables_charon_service])
#AM_CONDITIONAL([CHARON_SERVICE_ENABLED],[test "x$enables_charon_service" = "xyes"])

# Hopi service
AC_ARG_ENABLE(hopi_service, AC_HELP_STRING([--disable-hopi-service], [disable building Hopi Service]),
  [enables_hopi_service=$enableval],[])
if test "$enables_hopi_service" = "yes"; then
  if test "x$WIN32" = "xyes" ; then
    enables_hopi_service="no"
    AC_MSG_NOTICE([Hopi can't be built for WIN32 environment - disabling])
  fi
fi
AC_MSG_NOTICE([Hopi Service enabled: $enables_hopi_service])
AM_CONDITIONAL([HOPI_SERVICE_ENABLED],[test "x$enables_hopi_service" = "xyes"])

# Storage service
AC_ARG_ENABLE(storage_service, AC_HELP_STRING([--disable-storage-service], [disable building Storage service]),
  [enables_storage_service=$enableval],[])
if test "$enables_storage_service" = "yes"; then
  if test "x$enables_python" != "xyes"; then
    AC_MSG_NOTICE([WARNING: Missing or disabled Python - disabling STORAGE service])
    enables_storage_service="no"
  fi
fi
AC_MSG_NOTICE([STORAGE Service enabled: $enables_storage_service])
AM_CONDITIONAL([STORAGE_SERVICE_ENABLED],[test "x$enables_storage_service" = "xyes"])

# Janitor service
AC_ARG_ENABLE(janitor_service, AC_HELP_STRING([--disable-janitor-service], [disable building JANITOR Service]),
  [enables_janitor_service=$enableval],[])
if test "$enables_janitor_service" = "yes"; then
  if test "x$WIN32" = "xyes" ; then
    enables_janitor_service="no"
    AC_MSG_NOTICE([Janitor can't be built for WIN32 environment - disabling])
  fi
fi
AC_MSG_NOTICE([JANITOR Service enabled: $enables_janitor_service])
AM_CONDITIONAL([JANITOR_SERVICE_ENABLED],[test "x$enables_janitor_service" = "xyes"])
AM_CONDITIONAL([JANITOR_WEBSERVICE_ENABLED],[test "x$enables_janitor_service" = "xyes"])

# Cache service
AC_ARG_ENABLE(cache_service, AC_HELP_STRING([--disable-cache-service], [disable building cache service]),
  [enables_cache_service=$enableval],[])
if test "$enables_cache_service" = "yes"; then
  if test "x$WIN32" = "xyes" ; then
    enables_cache_service="no"
    AC_MSG_NOTICE([Cache service can't be built for WIN32 environment - disabling])
  fi
fi
AC_MSG_NOTICE([Cache service enabled: $enables_service_service])
AM_CONDITIONAL([CACHE_SERVICE_ENABLED],[test "x$enables_cache_service" = "xyes"])
AM_CONDITIONAL([CACHE_WEBSERVICE_ENABLED],[test "x$enables_cache_service" = "xyes"])


# Check for explicitely and implicitely disabled clients
AC_ARG_ENABLE(arclib_client, AC_HELP_STRING([--disable-arclib-client], [disable building Arclib (job management) client tools]),
  [enables_arclib_client=$enableval],[])
AC_MSG_NOTICE([Arclib client tools enabled: $enables_arclib_client])
AM_CONDITIONAL([ARCLIB_CLIENT_ENABLED],[test "x$enables_arclib_client" = "xyes"])

AC_ARG_ENABLE(charon_client, AC_HELP_STRING([--disable-charon-client], [disable building client for Charon service]),
  [enables_charon_client=$enableval],[])
AC_MSG_NOTICE([Charon client tool enabled: $enables_charon_client])
AM_CONDITIONAL([CHARON_CLIENT_ENABLED],[test "x$enables_charon_client" = "xyes"])

AC_ARG_ENABLE(credentials_client, AC_HELP_STRING([--disable-credentials-client], [disable building client tools for handling X.509 credentials]),
  [enables_credentials_client=$enableval],[])
AC_MSG_NOTICE([Credentials client tools enabled: $enables_credentials_client])
AM_CONDITIONAL([CREDENTIALS_CLIENT_ENABLED],[test "x$enables_credentials_client" = "xyes"])

AC_ARG_ENABLE(echo_client, AC_HELP_STRING([--disable-echo-client], [disable building client tools for communicationg with Echo service]),
  [enables_echo_client=$enableval],[])
AC_MSG_NOTICE([Echo client tool enabled: $enables_echo_client])
AM_CONDITIONAL([ECHO_CLIENT_ENABLED],[test "x$enables_echo_client" = "xyes"])

AC_ARG_ENABLE(data_client, AC_HELP_STRING([--disable-data-client], [disable building generic client tools for handling data]),
  [enables_data_client=$enableval],[])
AC_MSG_NOTICE([Data client tools enabled: $enables_data_client])
AM_CONDITIONAL([DATA_CLIENT_ENABLED],[test "x$enables_data_client" = "xyes"])

AC_ARG_ENABLE(isis_client, AC_HELP_STRING([--disable-isis-client], [disable building client tool for communicating ISIS]),
  [enables_isis_client=$enableval],[])
AC_MSG_NOTICE([ISIS client tool enabled: $enables_isis_client])
AM_CONDITIONAL([ISIS_CLIENT_ENABLED],[test "x$enables_isis_client" = "xyes"])

AC_ARG_ENABLE(jura_client, AC_HELP_STRING([--disable-jura-client], [disable building client tool for communicating JURA]),
  [enables_jura_client=$enableval],[])
AC_MSG_NOTICE([JURA client tool enabled: $enables_jura_client])
AM_CONDITIONAL([JURA_CLIENT_ENABLED],[test "x$enables_jura_client" = "xyes"])

AC_ARG_ENABLE(saml_client, AC_HELP_STRING([--disable-saml-client], [disable building client tool for communicating SAML-based VOMS service]),
  [enables_saml_client=$enableval],[])
if test "$enables_saml_client" = "yes"; then
  if test "$enables_xmlsec1" != "yes" ; then
    enables_saml_client="no"
    AC_MSG_NOTICE([SAML client requires xmlsec1 - disabling])
  fi
fi
AC_MSG_NOTICE([SAML client tool enabled: $enables_saml_client])
AM_CONDITIONAL([SAML_CLIENT_ENABLED],[test "x$enables_saml_client" = "xyes"])

AC_ARG_ENABLE(srm_client, AC_HELP_STRING([--disable-srm-client], [disable building client tools for communicating SRM services. This does not disable SRM DMC.]),
  [enables_srm_client=$enableval],[])
AC_MSG_NOTICE([SRM client tool enabled: $enables_srm_client])
AM_CONDITIONAL([SRM_CLIENT_ENABLED],[test "x$enables_srm_client" = "xyes"])

AC_ARG_ENABLE(wsrf_client, AC_HELP_STRING([--disable-wsrf-client], [disable building client tools for querying WSRF-enabled services.]),
  [enables_wsrf_client=$enableval],[])
AC_MSG_NOTICE([WSRF client tool enabled: $enables_wsrf_client])
AM_CONDITIONAL([WSRF_CLIENT_ENABLED],[test "x$enables_wsrf_client" = "xyes"])

AC_ARG_ENABLE(cache_client, AC_HELP_STRING([--disable-cache-client], [disable building client tools for cache service.]),
  [enables_cache_client=$enableval],[])
if test "$enables_cache_client" = "yes"; then
  if test "x$enables_python" != "xyes"; then
    enables_cache_client="no"
    AC_MSG_NOTICE([Cache client requires python - disabling])
  fi
fi
AC_MSG_NOTICE([Cache client tool enabled: $enables_cache_client])
AM_CONDITIONAL([CACHE_CLIENT_ENABLED],[test "x$enables_cache_client" = "xyes"])

AC_ARG_ENABLE(unicore_client, AC_HELP_STRING([--enable-unicore-client], [enables building UNICORE-related plugins.]),
  [enables_unicore_client=$enableval],[])
AC_MSG_NOTICE([UNICORE plugin(s) enabled: $enables_unicore_client])
AM_CONDITIONAL([UNICORE_ENABLED],[test "x$enables_unicore_client" = "xyes"])


# Check for consistency among disabled components
if test "$enables_hed" = "no"; then
  if test "$enables_a_rex_service" = "yes" -o \
          "$enables_hopi_service" = "yes" -o \
          "$enables_isis_service" = "yes" -o \
          "$enables_janitor_service" = "yes" -o \
          "$enables_charon_service" = "yes" -o \
          "$enables_storage_service" = "yes" -o \
          "$enables_cache_service" = "yes" -o \
          "$enables_arclib_client" = "yes" -o \
          "$enables_charon_client" = "yes" -o \
          "$enables_credentials_client" = "yes" -o \
          "$enables_echo_client" = "yes" -o \
          "$enables_data_client" = "yes" -o \
          "$enables_isis_client" = "yes" -o \
          "$enables_jura_client" = "yes" -o \
          "$enables_saml_client" = "yes" -o \
          "$enables_srm_client" = "yes" -o \
          "$enables_wsrf_client" = "yes" -o \
          "$enables_cache_client" = "yes"; then
    AC_MSG_ERROR(HED is needed for building any of the client or service tools. Please enable HED by using --enable-hed.)
  fi
fi
AM_CONDITIONAL([HED_ENABLED],[test "x$enables_hed" = "xyes"])

# A-Rex specific hack for backend scripts

pbs_bin_path=/usr/bin
pbs_log_path=/var/spool/pbs/server_logs
tmp_dir=/tmp
gnu_time=/usr/bin/time

case "${host}" in
    *linux*)
        nodename="/bin/hostname -f" ;;
    *)
        # hostname -f does not work on Solaris, OS X
        nodename="hostname" ;;
esac

arc_location=$prefix

AC_SUBST(arc_location)
AC_SUBST(pbs_bin_path)
AC_SUBST(pbs_log_path)
AC_SUBST(tmp_dir)
AC_SUBST(gnu_time)
AC_SUBST(nodename)

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
#DATER=`date -R`
DATER=`date +'%a, %d %b %Y %H:%M:%S %z'`
AC_SUBST(DATER)
SPECDATE=`LANG=C date +"%a %b %d %Y"`
AC_SUBST(SPECDATE)

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/hed/Makefile
                 src/hed/libs/client/Makefile
                 src/hed/libs/client/test/Makefile
                 src/hed/libs/common/Makefile
                 src/hed/libs/common/test/Makefile
                 src/hed/libs/credential/Makefile
                 src/hed/libs/credential/test/Makefile
                 src/hed/libs/credentialmod/Makefile
                 src/hed/libs/crypto/Makefile
                 src/hed/libs/cryptomod/Makefile
                 src/hed/libs/data/Makefile
                 src/hed/libs/data/cache-clean
                 src/hed/libs/data/cache-clean.1
                 src/hed/libs/data/cache-list
                 src/hed/libs/data/cache-list.1
                 src/hed/libs/data/test/Makefile
                 src/hed/libs/Makefile
                 src/hed/libs/loader/Makefile
                 src/hed/libs/loader/schema/Makefile
                 src/hed/libs/loader/test/Makefile
                 src/hed/libs/message/Makefile
                 src/hed/libs/message/test/Makefile
                 src/hed/libs/security/Makefile
                 src/hed/libs/security/ArcPDP/Makefile
                 src/hed/libs/security/ArcPDP/attr/Makefile
                 src/hed/libs/security/ArcPDP/policy/Makefile
                 src/hed/libs/security/ArcPDP/alg/Makefile
                 src/hed/libs/security/ArcPDP/fn/Makefile
                 src/hed/libs/credentialstore/Makefile
                 src/hed/libs/wsrf/Makefile
                 src/hed/libs/ws-addressing/Makefile
                 src/hed/libs/ws-security/Makefile
                 src/hed/libs/infosys/Makefile
                 src/hed/libs/infosys/schema/Makefile
                 src/hed/libs/infosys/test/Makefile
                 src/hed/libs/delegation/Makefile
                 src/hed/libs/delegation/test/Makefile
                 src/hed/libs/ws/Makefile
                 src/hed/libs/dbxml/Makefile
                 src/hed/libs/xmlsec/Makefile
                 src/hed/libs/globusutils/Makefile
                 src/hed/libs/python/Makefile
                 src/hed/libs/python/arcom/Makefile
                 src/hed/libs/python/arcom/store/Makefile
                 src/hed/daemon/Makefile
                 src/hed/daemon/scripts/Makefile
                 src/hed/daemon/schema/Makefile
                 src/hed/daemon/unix/Makefile
                 src/hed/daemon/win32/Makefile
                 src/hed/mcc/Makefile
                 src/hed/mcc/soap/Makefile
                 src/hed/mcc/tcp/Makefile
                 src/hed/mcc/tcp/schema/Makefile
                 src/hed/mcc/http/Makefile
                 src/hed/mcc/http/schema/Makefile
                 src/hed/mcc/tls/Makefile
                 src/hed/mcc/tls/schema/Makefile
                 src/hed/mcc/gsi/Makefile
                 src/hed/mcc/gsi/schema/Makefile
                 src/hed/mcc/msgvalidator/Makefile
                 src/hed/mcc/msgvalidator/schema/Makefile
                 src/hed/acc/Makefile
                 src/hed/acc/ARC0/Makefile
                 src/hed/acc/ARC1/Makefile
                 src/hed/acc/ARC1/test/Makefile
                 src/hed/acc/CREAM/Makefile
                 src/hed/acc/UNICORE/Makefile
                 src/hed/acc/Broker/Makefile
                 src/hed/acc/Broker/test/Makefile
                 src/hed/acc/PythonBroker/Makefile
                 src/hed/acc/JobDescriptionParser/Makefile
                 src/hed/acc/JobDescriptionParser/test/Makefile
                 src/hed/dmc/Makefile
                 src/hed/dmc/file/Makefile
                 src/hed/dmc/gridftp/Makefile
                 src/hed/dmc/http/Makefile
                 src/hed/dmc/arc/Makefile
                 src/hed/dmc/ldap/Makefile
                 src/hed/dmc/srm/Makefile
                 src/hed/dmc/srm/srmclient/Makefile
                 src/hed/dmc/lfc/Makefile
                 src/hed/dmc/rls/Makefile
                 src/hed/shc/Makefile
                 src/hed/shc/arcpdp/Makefile
                 src/hed/shc/arcpdp/schema/Makefile
                 src/hed/shc/xacmlpdp/Makefile
                 src/hed/shc/xacmlpdp/schema/Makefile
                 src/hed/shc/delegationpdp/Makefile
                 src/hed/shc/delegationpdp/schema/Makefile
                 src/hed/shc/gaclpdp/Makefile
                 src/hed/shc/pdpserviceinvoker/Makefile
                 src/hed/shc/pdpserviceinvoker/schema/Makefile
                 src/hed/shc/allowpdp/Makefile
                 src/hed/shc/denypdp/Makefile
                 src/hed/shc/simplelistpdp/Makefile
                 src/hed/shc/simplelistpdp/schema/Makefile
                 src/hed/shc/arcauthzsh/Makefile
                 src/hed/shc/arcauthzsh/schema/Makefile
                 src/hed/shc/usernametokensh/Makefile
                 src/hed/shc/usernametokensh/schema/Makefile
                 src/hed/shc/x509tokensh/Makefile
                 src/hed/shc/x509tokensh/schema/Makefile
                 src/hed/shc/samltokensh/Makefile
                 src/hed/shc/samltokensh/schema/Makefile
                 src/hed/shc/saml2sso_assertionconsumersh/Makefile
                 src/hed/shc/delegationsh/Makefile
                 src/hed/shc/delegationsh/schema/Makefile
                 src/hed/identitymap/Makefile
                 src/hed/identitymap/schema/Makefile
                 src/libs/Makefile
                 src/libs/data-staging/Makefile
                 src/libs/data-staging/test/Makefile
                 src/services/Makefile
                 src/services/a-rex/Makefile
                 src/services/a-rex/a-rex
                 src/services/a-rex/arex.xml.example
                 src/services/a-rex/arex_minimalistic.xml.example
                 src/services/a-rex/arex_secure.xml.example
                 src/services/a-rex/arex_xmlonly.xml.example
                 src/services/a-rex/arex_showcase.xml.example
                 src/services/a-rex/arex_janitor.xml.example
                 src/services/a-rex/config/Makefile
                 src/services/a-rex/grid-manager/Makefile
                 src/services/a-rex/grid-manager/conf/Makefile
                 src/services/a-rex/grid-manager/files/Makefile
                 src/services/a-rex/grid-manager/jobs/Makefile
                 src/services/a-rex/grid-manager/jobplugin/Makefile
                 src/services/a-rex/grid-manager/loaders/Makefile
                 src/services/a-rex/grid-manager/log/Makefile
                 src/services/a-rex/grid-manager/mail/Makefile
                 src/services/a-rex/grid-manager/misc/Makefile
                 src/services/a-rex/grid-manager/run/Makefile
                 src/services/a-rex/infoproviders/Makefile
                 src/services/a-rex/infoproviders/CEinfo.pl
                 src/services/a-rex/infoproviders/qju.pl
                 src/services/a-rex/infoproviders/cluster.pl
                 src/services/a-rex/infoproviders/se.pl
                 src/services/a-rex/infoproviders/test/Makefile
                 src/services/a-rex/ldif/Makefile
                 src/services/a-rex/lrms/Makefile
                 src/services/a-rex/lrms/submit_common.sh
                 src/services/a-rex/lrms/scan_common.sh
                 src/services/a-rex/lrms/condor/Makefile
                 src/services/a-rex/lrms/condor/finish-condor-job
                 src/services/a-rex/lrms/condor/scan-condor-job
                 src/services/a-rex/lrms/condor/cancel-condor-job
                 src/services/a-rex/lrms/condor/submit-condor-job
                 src/services/a-rex/lrms/fork/Makefile
                 src/services/a-rex/lrms/fork/scan-fork-job
                 src/services/a-rex/lrms/fork/submit-fork-job
                 src/services/a-rex/lrms/fork/cancel-fork-job
                 src/services/a-rex/lrms/ll/Makefile
                 src/services/a-rex/lrms/ll/submit-ll-job
                 src/services/a-rex/lrms/ll/cancel-ll-job
                 src/services/a-rex/lrms/ll/scan-ll-job
                 src/services/a-rex/lrms/lsf/Makefile
                 src/services/a-rex/lrms/lsf/submit-lsf-job
                 src/services/a-rex/lrms/lsf/cancel-lsf-job
                 src/services/a-rex/lrms/lsf/scan-lsf-job
                 src/services/a-rex/lrms/pbs/Makefile
                 src/services/a-rex/lrms/pbs/submit-pbs-job
                 src/services/a-rex/lrms/pbs/cancel-pbs-job
                 src/services/a-rex/lrms/pbs/scan-pbs-job
                 src/services/a-rex/lrms/pbs/configure-pbs-env.sh
                 src/services/a-rex/lrms/sge/Makefile
                 src/services/a-rex/lrms/sge/submit-sge-job
                 src/services/a-rex/lrms/sge/scan-sge-job
                 src/services/a-rex/lrms/sge/cancel-sge-job
                 src/services/a-rex/lrms/slurm/Makefile
                 src/services/a-rex/lrms/slurm/submit-SLURM-job
                 src/services/a-rex/lrms/slurm/scan-SLURM-job
                 src/services/a-rex/lrms/slurm/cancel-SLURM-job
                 src/services/a-rex/lrms/gridfactory/Makefile
                 src/services/a-rex/lrms/gridfactory/cancel-gridfactory-job
                 src/services/a-rex/lrms/gridfactory/configure-gridfactory-env.sh
                 src/services/a-rex/lrms/gridfactory/scan-gridfactory-job
                 src/services/a-rex/lrms/gridfactory/submit-gridfactory-job
                 src/services/a-rex/lrms/dgbridge/Makefile
                 src/services/a-rex/lrms/dgbridge/submit-DGBridge-job
                 src/services/a-rex/lrms/dgbridge/scan-DGBridge-job
                 src/services/a-rex/lrms/dgbridge/cancel-DGBridge-job
                 src/services/a-rex/logger/Makefile
                 src/services/a-rex/urlogger/Makefile
                 src/services/a-rex/schema/Makefile
                 src/services/cache_service/Makefile
                 src/services/cache_service/cache-service
                 src/services/charon/Makefile
                 src/services/charon/schema/Makefile
                 src/services/charon/charon_service.xml.example
                 src/services/gridftpd/Makefile
                 src/services/gridftpd/gridftpd.init
                 src/services/gridftpd/auth/Makefile
                 src/services/gridftpd/conf/Makefile
                 src/services/gridftpd/external/Makefile
                 src/services/gridftpd/external/gacl/Makefile
                 src/services/gridftpd/misc/Makefile
                 src/services/gridftpd/run/Makefile
                 src/services/gridftpd/fileplugin/Makefile
                 src/services/gridftpd/gaclplugin/Makefile
                 src/services/hopi/Makefile
                 src/services/hopi/hopi_service.xml.example
                 src/services/isis/Makefile
                 src/services/isis/schema/Makefile
                 src/services/ldap-infosys/Makefile
                 src/services/ldap-infosys/grid-infosys
                 src/services/ldap-infosys/grid-infosys-bdii4
                 src/services/ldap-infosys/grid-infosys-bdii5
                 src/services/ldap-infosys/giis/Makefile
                 src/services/ldap-monitor/Makefile
                 src/services/ldap-monitor/ldap-monitor
                 src/services/ldap-monitor/README
                 src/services/ldap-monitor/man/Makefile
                 src/services/ldap-monitor/man/ldap-monitor.7
                 src/services/ldap-monitor/includes/Makefile
                 src/services/ldap-monitor/cache/Makefile
                 src/services/ldap-monitor/mon-icons/Makefile
                 src/services/ldap-monitor/lang/Makefile
                 src/services/ws-monitor/Makefile
                 src/services/ws-monitor/ws-monitor
                 src/services/ws-monitor/README
                 src/services/ws-monitor/man/Makefile
                 src/services/ws-monitor/man/ws-monitor.7
                 src/services/ws-monitor/includes/Makefile
                 src/services/ws-monitor/cache/Makefile
                 src/services/ws-monitor/mon-icons/Makefile
                 src/services/ws-monitor/lang/Makefile
                 src/services/slcs/Makefile
                 src/services/slcs/schema/Makefile
                 src/services/storage/Makefile
                 src/services/storage/storage_service.xml.example
                 src/services/storage/chelonia.1
                 src/services/storage/librarian/Makefile
                 src/services/storage/librarian/schema/Makefile
                 src/services/storage/cli/Makefile
                 src/services/storage/shepherd/Makefile
                 src/services/storage/shepherd/schema/Makefile
                 src/services/storage/ahash/Makefile
                 src/services/storage/ahash/schema/Makefile
                 src/services/storage/bartender/Makefile
                 src/services/storage/bartender/schema/Makefile
                 src/services/storage/bartender/gateway/Makefile
                 src/services/storage/fuse/Makefile
                 src/services/janitor/interfaces/commandline/Makefile
                 src/services/janitor/interfaces/commandline/janitor
                 src/services/janitor/interfaces/webservice/Makefile
                 src/services/janitor/interfaces/Makefile
                 src/services/janitor/interfaces/client/Makefile
                 src/services/janitor/module/Catalog/Makefile
                 src/services/janitor/module/Makefile
                 src/services/janitor/schema/Makefile
                 src/services/janitor/resources/Makefile
                 src/services/janitor/Makefile
                 src/services/echo_java/Makefile
                 src/services/echo_python/Makefile
                 src/services/wrappers/Makefile
                 src/services/wrappers/java/Makefile
                 src/services/wrappers/java/schema/Makefile
                 src/services/wrappers/python/Makefile
                 src/services/wrappers/python/schema/Makefile
                 src/clients/Makefile
                 src/clients/data/Makefile
                 src/clients/data/arccp.1
                 src/clients/data/arcls.1
                 src/clients/data/arcrm.1
                 src/clients/echo/Makefile
                 src/clients/echo/arcecho.1
                 src/clients/charon/Makefile
                 src/clients/charon/arcdecision.1
                 src/clients/isis/Makefile
                 src/clients/isis/isistest.1
                 src/clients/credentials/Makefile
                 src/clients/credentials/arcproxy.1
                 src/clients/credentials/arcslcs.1
                 src/clients/saml/Makefile
                 src/clients/saml/saml_assertion_init.1
                 src/clients/arclib/Makefile
                 src/clients/arclib/arcstat.1
                 src/clients/arclib/arcinfo.1
                 src/clients/arclib/arcsub.1
                 src/clients/arclib/arcclean.1
                 src/clients/arclib/arckill.1
                 src/clients/arclib/arcget.1
                 src/clients/arclib/arccat.1
                 src/clients/arclib/arcresub.1
                 src/clients/arclib/arcmigrate.1
                 src/clients/arclib/arcsync.1
                 src/clients/arclib/arcrenew.1
                 src/clients/arclib/arcresume.1
                 src/clients/wsrf/arcwsrf.1
                 src/clients/srm/Makefile
                 src/clients/srm/arcsrmping.1
                 src/clients/jura/Makefile
                 src/clients/jura/jura.1
                 src/clients/wsrf/Makefile
                 src/clients/cache/Makefile
                 src/tests/Makefile
                 src/tests/echo/Makefile
                 src/tests/echo/perftest.1
                 src/tests/echo/echo_service.xml.example
                 src/tests/echo/schema/Makefile
                 src/tests/policy-delegation/Makefile
                 src/tests/delegation/Makefile
                 src/tests/translator/Makefile
                 src/tests/xpath/Makefile
                 src/tests/arcpolicy/Makefile
                 src/tests/perf/Makefile
                 src/tests/client/Makefile
                 src/utils/ngclient2arc.1
                 src/utils/hed/wsdl2hed.1
                 src/utils/hed/arcplugin.1
                 src/utils/hed/Makefile
                 src/utils/gridmap/nordugridmap.cron
                 src/utils/gridmap/Makefile
                 src/utils/testing/ngconfigtest.1
                 src/utils/testing/Makefile
                 src/utils/Makefile
                 src/doc/Makefile
                 src/doc/arc.conf.5
                 swig/Makefile
                 java/Makefile
                 python/Makefile
                 po/Makefile.in
                 include/Makefile
                 debian/Makefile
                 debian/changelog.deb
                 release_notes/Makefile
                 nordugrid-arc.spec
                 mingw32-nordugrid-arc.spec
                 src/hed/daemon/arched.8
                 src/hed/daemon/scripts/arched
                 src/services/janitor/interfaces/commandline/janitor.8
                 src/services/janitor/interfaces/commandline/rjanitor.c
                 arcbase.pc
                 nsis/Makefile
                 nsis/arc.nsis
                 src/doxygen/Makefile
])
AC_OUTPUT

AC_MSG_RESULT([
Unit testing:       ${enables_cppunit}
Java binding:       ${enables_java}
Python binding:     ${enables_python} ($PYTHON_VERSION)

Available third-party features:

RLS:                ${enables_rls}
GridFTP:            ${enables_gridftp}
LFC:                ${enables_lfc}
MYSQL CLIENT LIB:   ${enables_mysql}
LDAP:               ${enables_ldap}
xmlsec1:            ${enables_xmlsec1}
ARGUS:              ${enables_argus}
GACL:               ${enables_gacl}

Included components:
HED:                ${enables_hed}

A-REX service:      ${enables_a_rex_service}
GRIDFTPD service:   ${enables_gridftpd_service}
ISIS service:       ${enables_isis_service}
LDAP Info service:  ${enables_ldap_service}
GIIS service:       ${enables_giis_service}
HOPI service:       ${enables_hopi_service}
STORAGE service:    ${enables_storage_service}
JANITOR service:    ${enables_janitor_service}
CACHE service:      ${enables_cache_service}

ARCLIB clients:     ${enables_arclib_client}
ISIS client:        ${enables_isis_client}
CHARON client:      ${enables_charon_client}
DATA clients:       ${enables_data_client}
SRM ping client:    ${enables_srm_client}
CREDENTIAL clients: ${enables_credentials_client}
ECHO client:        ${enables_echo_client}
JURA client:        ${enables_jura_client}
SAML VOMS client:   ${enables_saml_client}
WSRF client:        ${enables_wsrf_client}
CACHE client:       ${enables_cache_client}

UNICORE client (ACC): ${enables_unicore_client}
SRM client (DMC):   ${enables_srm_dmc}
GSI channel (MCC):  ${enables_gsi_mcc}

Documentation:      ${enables_doc}

Monitoring:
LDAP Monitor        ${enables_ldap_monitor}
WS Monitor          ${enables_ws_monitor}
])

if test "$enables_hed" = "yes"; then
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
fi

