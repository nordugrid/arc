#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.56)
AC_CANNONICAL_SYSTEM
AC_INIT([arc],[0.9.0],[http://bugzilla.nordugrid.org/])
AM_INIT_AUTOMAKE([foreign tar-pax])
AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])

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

docdir='${datadir}/doc/${PACKAGE}'
pkglibexecdir='${libexecdir}/${PACKAGE}'
AC_SUBST(docdir)
AC_SUBST(pkglibexecdir)

ARC_API
ARC_RELATIVE_PATHS

AC_ARG_WITH(sysv-scripts-location,
  AC_HELP_STRING([--with-sysv-scripts-location=<PATH>], [Location of the SYSV init scripts. [[autodetect]]]),
  [ initddir="$withval" ],
  [
    initddir=
    for i in init.d rc.d/init.d rc.d; do
      if test -d "/etc/$i" -a ! -h "/etc/$i" ; then
        initddir="$sysconfdir/$i"
        break
      fi
    done

    if test -z "$initddir" && test -d "/Library/LaunchDaemons" -a ! -h " /Library/LaunchDaemons"; then
      initddir="/Library/LaunchDaemons"
    fi

    # We need to find a solution for WIN32

    if test -z "$initddir" && test -d "/c" -a ! -h " /c"; then
        initddir="/c"
    fi

    if test -z "$initddir"; then
      AC_MSG_ERROR(could not find a suitable location for the SYSV init scripts)
    fi
  ]
)
AC_MSG_RESULT($initddir)
AC_SUBST(initddir)

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
enables_cppunit=yes
enables_gsoap=yes
enables_java=yes
enables_ldap=yes
enables_lfc=no
enables_mysql=no
enables_python=yes
enables_swig=yes
enables_xmlsec1=yes
#  Libraries and plugins
# Currently no fine-grained choice is supported.
# Also this variable is used to check if source
# build is neded at all because no component can
# be built without HED.
enables_hed=yes
#  Services
enables_a_rex_service=yes
enables_compiler_service=yes
enables_hopi_service=yes
enables_isis_service=yes
enables_janitor_service=yes
enables_paul_service=yes
enables_sched_service=yes
enables_charon_service=yes
enables_storage_service=yes
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
#  Documentation
enables_doc=yes

# Handle group enable/disable switches
AC_ARG_ENABLE(all, AC_HELP_STRING([--disable-all], [disables all buildable components. Can be overwritten with --enable-* for group or specific component. It is also possible to use --enable-all to overwrite defaults.]),
[
enables_a_rex_service=$enableval
enables_compiler_service=$enableval
enables_hopi_service=$enableval
enables_isis_service=$enableval
enables_janitor_service=$enableval
enables_paul_service=$enableval
enables_sched_service=$enableval
enables_charon_service=$enableval
enables_storage_service=$enableval
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
enables_hed=$enableval
enables_java=$enableval
enables_python=$enableval
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
enables_compiler_service=$enableval
enables_hopi_service=$enableval
enables_isis_service=$enableval
enables_janitor_service=$enableval
enables_paul_service=$enableval
enables_sched_service=$enableval
enables_charon_service=$enableval
enables_storage_service=$enableval
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
AM_CONDITIONAL([SWIG_ENABLED],[test "$SWIG" != "x"])

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
  AC_ARG_WITH(jdk, AC_HELP_STRING([--with-jdk=(JDK)], [path to SUN JDK. If unset the system JDK will be used]))
  if test "X$with_jdk" != "X"; then

    # user-specified SUN JDK

    JAVA="$with_jdk/bin/java"
    JAVAC="$with_jdk/bin/javac"
    JAR="$with_jdk/bin/jar"
    JDK_CFLAGS="-I$with_jdk/include"
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
    AC_CHECK_HEADERS(jni.h,[jni_h="yes"],[jni_h="no"])
    CPPFLAGS=$SAVE_CPPFLAGS

  else

    # System JDK

    AC_CHECK_HEADERS(jni.h JavaVM/jni.h,[jni_h="yes"; break],[jni_h="no"])

  fi

  AC_CHECK_PROGS(JAVA, java gij)
  AC_CHECK_PROGS(JAVAC, gcj javac ecj)
  AC_CHECK_PROGS(JAR, fastjar jar)

  if test "X$JAVAC" != "X" && test "X`basename $JAVAC`" = "Xgcj"; then
    JAVAC_FLAGS=-C
  fi

  JAVAWRAP=no
  if test -f java/arc_wrap.cpp || test "X$SWIG" != "X" ; then
    JAVAWRAP=yes
  else
    AC_MSG_NOTICE([Missing pre-compiled Java wrapper or SWIG - skipping Java bindings])
  fi

  if test "x$jni_h" != "xyes"; then
    AC_MSG_NOTICE([Missing Java headers - skipping Java bindings])
    enables_java="no"
  elif test "x$JAVAWRAP" != "xyes"; then
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
    PYTHON_VERSION=`$PYTHON -c 'import sys; print sys.version[:3]'`

    case $host in
       *-*-mingw*)
          SAVE_PYTHON_VERSION=$PYTHON_VERSION
          PYTHON_VERSION=`echo $PYTHON_VERSION | sed 's/\.//g'`
       ;;
    esac

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

    AC_CHECK_LIB([python$PYTHON_VERSION], [Py_InitializeEx],[
        AC_MSG_NOTICE([Python includes functionality of skipping initialization registration of signal handlers])
        AC_DEFINE(HAVE_PYTHON_INITIALIZE_EX,,[Define if you have Py_InitializeEx function])],[AC_MSG_NOTICE([Python does not include functionality of skipping initialization registration of signal handlers, since its version is below 2.4])])

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
AM_CONDITIONAL([PYTHON_SERVICE],[test ! "x$PYTHON_VERSION" '<' "x2.4"])




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

# check gSOAP
gsoap=no
if test "$enables_hed" = "yes"; then
AC_ARG_ENABLE(gsoap, AC_HELP_STRING([--disable-gsoap], [disable the code dependant on gSOAP]),[enables_gsoap=$enableval],[])
if test "$enables_gsoap" = "yes"; then
  PKG_CHECK_MODULES(GSOAP, [gsoap++ >= 2.7], [gsoap=yes], [gsoap=no])
  AC_SUBST(GSOAP_CFLAGS)
  AC_SUBST(GSOAP_LIBS)
  SOAPCPP2=
  WSDL2H=
  if test "$gsoap" = 'yes'; then
    AC_PATH_PROG(SOAPCPP2, soapcpp2, [])
    AC_PATH_PROG(WSDL2H, wsdl2h, [])
    if test "x$SOAPCPP2" = "x"; then
      gsoap=no
      AC_MSG_WARN([Missing soapcpp2 utility - maybe You have to add it's location to PATH])
    elif test "x$WSDL2H" = "x"; then
      gsoap=no
      AC_MSG_WARN([Missing wsdl2h utility - maybe You have to add it's location to PATH])
    else
      # gSOAP package does not report third number of version. Hence we
      # have to extract it
      AC_MSG_CHECKING(for gSOAP version)
      gsoap_version=`${SOAPCPP2} -v 2>&1 | sed -e 's/.*version \([[^ ]]*\).*/\1/' -e 't new' -e 's/^.*Compiler for C and C++ *\([[^ ]]*\).*/\1/' -e 't matched' -e 's/.*//' -e ':matched' -e ':new' | grep '.'`
      if test "x$gsoap_version" = "x"; then
        gsoap=no
        AC_MSG_WARN([Could not determine version.])
      else
        gsoap_version_major=`echo "$gsoap_version" | sed -e 's/^\([[^\.]]\).*/\1/' -e 't match' -e 's/.*/0/' -e ':match'`
        gsoap_version_minor=`echo "$gsoap_version" | sed -e 's/^[[^\.]]*\.\([[^\.]]*\).*/\1/' -e 't match' -e 's/.*/0/' -e ':match'`
        gsoap_version_subminor=`echo "$gsoap_version" | sed -e 's/^[[^\.]]*\.[[^\.]]*\.\([[0-9]]*\).*/\1/' -e 't match' -e 's/.*/0/' -e ':match'`
        AC_MSG_RESULT($gsoap_version_major.$gsoap_version_minor.$gsoap_version_subminor)
        if test "$gsoap_version_major" -lt "2"; then
          gsoap=no
          AC_MSG_WARN([gSOAP with version below 2.7.2 is not supported])
        fi
        if test "$gsoap_version_major" -eq "2"; then
          if test "$gsoap_version_minor" -lt "7" ; then
            gsoap=no
            AC_MSG_WARN([gSOAP with version below 2.7.2 is not supported])
          fi
          if test "$gsoap_version_minor" -eq "7" && test "$gsoap_version_subminor" -lt "2" ; then
            gsoap=no
            AC_MSG_WARN([found gSOAP version $gsoap_version_major.$gsoap_version_minor.$gsoap_version_subminor - gSOAP with version below 2.7.2 is not supported anymore])
          fi
        fi
      fi
    fi
  fi

  if test "$gsoap" = 'yes'; then

    AC_MSG_CHECKING(for gSOAP pecularities)

cat > gsoaptest.xsd <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<xsd:schema xmlns:xsd="http://www.w3.org/2001/XMLSchema">
  <xsd:complexType name="X">
    <xsd:sequence>
      <xsd:element ref="Y"/>
    </xsd:sequence>
  </xsd:complexType>
  <xsd:element name="Y" type="xsd:string"/>
</xsd:schema>
EOF

    ${WSDL2H} -s gsoaptest.xsd

    ${SOAPCPP2} -p gsoaptest gsoaptest.h

    AC_LANG_SAVE
    AC_LANG_CPLUSPLUS
    CXXFLAGS_SAVE=$CXXFLAGS
    CXXFLAGS="$CXXFLAGS $GSOAP_CFLAGS"
    AC_TRY_COMPILE([#include "gsoaptestStub.h"],
        [ns1__X x; x.Y = "";],
        [use_namespaces="no"],
        [AC_TRY_COMPILE([#include "gsoaptestStub.h"],
                        [ns1__X x; x.ns1__Y = "";],
                        [use_namespaces="yes"],
                        [gsoap=no])
        ]
    )

    CXXFLAGS=$CXXFLAGS_SAVE
    AC_LANG_RESTORE

    rm -rf gsoaptest*

    if test "x$use_namespaces" = "xyes"; then
      AC_DEFINE(GSOAP_ALWAYS_USES_NAMESPACES_FOR_MEMBER_NAMES,,[define if gSOAP prepends names of class members with namespace])
    fi

    AC_MSG_RESULT(done)

  fi

  if test ! "$gsoap" = 'yes'; then
    SOAPCPP2=
    WSDL2H=
    GSOAP_CFLAGS=
    GSOAP_LIBS=
  fi

  AC_SUBST(SOAPCPP2)
  AC_SUBST(WSDL2H)
  AC_SUBST(GSOAP_CFLAGS)
  AC_SUBST(GSOAP_LIBS)
  enables_gsoap="$gsoap"
fi
fi
AM_CONDITIONAL(HAVE_GSOAP, test "x$gsoap" = "xyes")

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

# check zlib
if test "$enables_hed" = "yes"; then
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
fi

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
AC_ARG_ENABLE(lfc, AC_HELP_STRING([--enable-lfc], [enable the LFC support, default is disable]),[enables_lfc="$enableval"],[])
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
  AC_CHECK_LIB([lfc], [C__serrno], [], [enables_lfc="no"]))
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

enables_srm_dmc=no
if test "$enables_hed" = "yes"; then
if test "x$gsoap" = 'xyes'; then
  if test ! "x$GLOBUS_IO_LIBS" = "x"; then
    enables_srm_dmc=yes
  fi
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
enables_saml=no
if test x$XMLSEC_INSTALLED = xyes ; then enables_saml=yes; fi

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
fi
AM_CONDITIONAL([LDAP_ENABLED], test x$LDAP = xyes)

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
AC_CHECK_FUNCS([acl dup2 floor ftruncate gethostname getdomainname getpid gmtime_r lchown localtime_r memchr memmove memset mkdir mkfifo regcomp rmdir select setenv socket strcasecmp strchr strcspn strdup strerror strncasecmp strstr strtol strtoul strtoull timegm tzset unsetenv getopt_long_only])
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

# Check if user asks to skip documentation build
AC_ARG_ENABLE(doc, AC_HELP_STRING([--disable-doc], [disable building documentation (requires doxygen and pdflatex)]),[enables_doc=$enableval],[])
if test "x$enables_doc" = "xyes"; then
  AC_CHECK_PROGS(PDFLATEX, pdflatex)
  AC_CHECK_PROGS(DOXYGEN, doxygen)
  AC_CHECK_PROGS(DOT, dot)
  if test "x$PDFLATEX" = "x"; then
    enables_doc="no"
    AC_MSG_NOTICE([WARNING: Missing pdflatex - documentation won't be built])
  elif test "x$DOXYGEN" = "x"; then
    enables_doc="no"
    AC_MSG_NOTICE([WARNING: Missing doxygen - documentation won't be built])
  elif test "x$DOT" = "x"; then
    enables_doc="no"
    AC_MSG_NOTICE([WARNING: Missing dot - documentation won't be built])
  fi
fi
AC_MSG_NOTICE([Documentation enabled: $enables_doc])
AM_CONDITIONAL([DOC_ENABLED],[test "x$enables_doc" = "xyes"])


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

# Charon service
AC_ARG_ENABLE(charon_service, AC_HELP_STRING([--disable-charon-service], [disable building Charon Service]),
  [enables_charon_service=$enableval],[])
if test "$enables_charon_service" = "yes"; then
  if test "x$WIN32" = "xyes" ; then
    enables_charon_service="no"
    AC_MSG_NOTICE([Charon can't be built for WIN32 environment - disabling])
  fi
fi
AC_MSG_NOTICE([Charon Service enabled: $enables_charon_service])
AM_CONDITIONAL([CHARON_SERVICE_ENABLED],[test "x$enables_charon_service" = "xyes"])

# Compiler service
AC_ARG_ENABLE(compiler_service, AC_HELP_STRING([--disable-compiler-service], [disable building Compiler Service]),
  [enables_compiler_service=$enableval],[])
if test "$enables_compiler_service" = "yes"; then
  if test "x$WIN32" = "xyes" ; then
    enables_compiler_service="no"
    AC_MSG_NOTICE([Compiler can't be built for WIN32 environment - disabling])
  fi
fi
AC_MSG_NOTICE([Compiler Service enabled: $enables_compiler_service])
AM_CONDITIONAL([COMPILER_SERVICE_ENABLED],[test "x$enables_compiler_service" = "xyes"])

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

# Paul service
AC_ARG_ENABLE(paul_service, AC_HELP_STRING([--disable-paul-service], [disable building PAUL Service]),
  [enables_paul_service=$enableval],[])
if test "$enables_paul_service" = "yes"; then
  if test "x$WIN32" = "xyes" ; then
    enables_paul_service="no"
    AC_MSG_NOTICE([PAUL can't be built for WIN32 environment - disabling])
  fi
fi
AC_MSG_NOTICE([PAUL Service enabled: $enables_paul_service])
AM_CONDITIONAL([PAUL_SERVICE_ENABLED],[test "x$enables_paul_service" = "xyes"])

# Sched service
AC_ARG_ENABLE(sched_service, AC_HELP_STRING([--disable-sched-service], [disable building SCHED Service]),
  [enables_sched_service=$enableval],[])
if test "$enables_sched_service" = "yes"; then
  if test "x$WIN32" = "xyes" ; then
    enables_sched_service="no"
    AC_MSG_NOTICE([SCHED can't be built for WIN32 environment - disabling])
  fi
fi
if test "$enables_sched_service" = "yes"; then
  if test "$DBCXX_LIBS" = ""; then
    enables_sched_service="no"
    AC_MSG_NOTICE([WARNING: Missing db_cxx - disabling SCHED Service])
  fi
fi
AC_MSG_NOTICE([SCHED Service enabled: $enables_sched_service])
AM_CONDITIONAL([SCHED_SERVICE_ENABLED],[test "x$enables_sched_service" = "xyes"])

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


# Check for consistency among disabled components
if test "$enables_hed" = "no"; then
  if test "$enables_a_rex_service" = "yes" -o \
          "$enables_compiler_service" = "yes" -o \
          "$enables_hopi_service" = "yes" -o \
          "$enables_isis_service" = "yes" -o \
          "$enables_janitor_service" = "yes" -o \
          "$enables_paul_service" = "yes" -o \
          "$enables_sched_service" = "yes" -o \
          "$enables_charon_service" = "yes" -o \
          "$enables_storage_service" = "yes" -o \
          "$enables_arclib_client" = "yes" -o \
          "$enables_charon_client" = "yes" -o \
          "$enables_credentials_client" = "yes" -o \
          "$enables_echo_client" = "yes" -o \
          "$enables_data_client" = "yes" -o \
          "$enables_isis_client" = "yes" -o \
          "$enables_jura_client" = "yes" -o \
          "$enables_saml_client" = "yes" -o \
          "$enables_srm_client" = "yes" -o \
          "$enables_wsrf_client" = "yes"; then
    AC_MSG_ERROR(HED is needed for building any of client or service tools. Please enabled HED by using --enable-hed.)
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

AC_CONFIG_SUBDIRS([doc])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/hed/Makefile
                 src/hed/libs/client/Makefile
                 src/hed/libs/client/test/Makefile
                 src/hed/libs/common/Makefile
                 src/hed/libs/common/test/Makefile
                 src/hed/libs/credential/Makefile
                 src/hed/libs/crypto/Makefile
                 src/hed/libs/data/Makefile
                 src/hed/libs/data/cache-clean
                 src/hed/libs/data/cache-clean.1
                 src/hed/libs/data/cache-list
                 src/hed/libs/data/cache-list.1
                 src/hed/libs/data/test/Makefile
                 src/hed/libs/Makefile
                 src/hed/libs/loader/Makefile
                 src/hed/libs/loader/schema/Makefile
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
                 src/hed/acc/CREAM/Makefile
                 src/hed/acc/UNICORE/Makefile
                 src/hed/acc/Broker/Makefile
                 src/hed/acc/PythonBroker/Makefile
                 src/hed/dmc/Makefile
                 src/hed/dmc/file/Makefile
                 src/hed/dmc/gridftp/Makefile
                 src/hed/dmc/http/Makefile
                 src/hed/dmc/arc/Makefile
                 src/hed/dmc/ldap/Makefile
                 src/hed/dmc/srm/Makefile
                 src/hed/dmc/srm/https/Makefile
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
                 src/services/Makefile
                 src/services/a-rex/Makefile
                 src/services/a-rex/a-rex
                 src/services/a-rex/arex.xml.example
                 src/services/a-rex/arex_minimalistic.xml.example
                 src/services/a-rex/arex_secure.xml.example
                 src/services/a-rex/arex_xmlonly.xml.example
                 src/services/a-rex/arex_showcase.xml.example
                 src/services/a-rex/config/Makefile
                 src/services/a-rex/grid-manager/Makefile
                 src/services/a-rex/grid-manager/conf/Makefile
                 src/services/a-rex/grid-manager/files/Makefile
                 src/services/a-rex/grid-manager/jobs/Makefile
                 src/services/a-rex/grid-manager/loaders/Makefile
                 src/services/a-rex/grid-manager/log/Makefile
                 src/services/a-rex/grid-manager/mail/Makefile
                 src/services/a-rex/grid-manager/misc/Makefile
                 src/services/a-rex/grid-manager/run/Makefile
                 src/services/a-rex/infoproviders/Makefile
                 src/services/a-rex/infoproviders/CEinfo.pl
                 src/services/a-rex/infoproviders/test/Makefile
                 src/services/a-rex/ldif/Makefile
                 src/services/a-rex/lrms/Makefile
                 src/services/a-rex/lrms/submit_common.sh
                 src/services/a-rex/lrms/condor/Makefile
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
                 src/services/a-rex/logger/Makefile
                 src/services/a-rex/schema/Makefile
                 src/services/charon/Makefile
                 src/services/charon/schema/Makefile
                 src/services/charon/charon_service.xml.example
                 src/services/compiler/Makefile
                 src/services/compiler/schema/Makefile
                 src/services/hopi/Makefile
                 src/services/hopi/hopi_service.xml.example
                 src/services/arex2/Makefile
                 src/services/isis/Makefile
                 src/services/isis/schema/Makefile
                 src/services/saml/Makefile
                 src/services/saml/schema/Makefile
                 src/services/slcs/Makefile
                 src/services/slcs/schema/Makefile
                 src/services/delegation/Makefile
                 src/services/sched/Makefile
                 src/services/sched/schema/Makefile
                 src/services/sched/manage_jobq.8
                 src/services/paul/Makefile
                 src/services/paul/schema/Makefile
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
                 src/utils/wsdl2hed.1
                 src/utils/arcplugin.1
                 src/utils/Makefile
                 swig/Makefile
                 java/Makefile
                 python/Makefile
                 po/Makefile.in
                 include/Makefile
                 debian/Makefile
                 debian/changelog.deb
                 arc.spec
                 mingw32-arc.spec
                 src/hed/daemon/arched.8
                 src/services/janitor/interfaces/commandline/janitor.8
                 src/services/janitor/interfaces/commandline/rjanitor.c
                 arcbase.pc
                 nsis/Makefile
                 nsis/arc.nsis
                 src/hed/profiles/CheloniaAllServicesCentralizedAHash/CheloniaAllServicesCentralizedAHash.ini
                 src/hed/profiles/CheloniaAllServicesCentralizedAHash/CheloniaAllServicesCentralizedAHash.xml
                 src/hed/profiles/CheloniaAllServicesCentralizedAHashWithISIS/CheloniaAllServicesCentralizedAHashWithISIS.ini
                 src/hed/profiles/CheloniaAllServicesCentralizedAHashWithISIS/CheloniaAllServicesCentralizedAHashWithISIS.xml
                 src/hed/profiles/CheloniaAllServicesReplicatedAHash/CheloniaAllServicesReplicatedAHash.ini
                 src/hed/profiles/CheloniaAllServicesReplicatedAHash/CheloniaAllServicesReplicatedAHash.xml
                 src/hed/profiles/CheloniaAllServicesReplicatedAHashGatewayVOMSWithISIS/CheloniaAllServicesReplicatedAHashGatewayVOMSWithISIS.ini
                 src/hed/profiles/CheloniaAllServicesReplicatedAHashGatewayVOMSWithISIS/CheloniaAllServicesReplicatedAHashGatewayVOMSWithISIS.xml
                 src/hed/profiles/CheloniaAllServicesReplicatedAHashWithISIS/CheloniaAllServicesReplicatedAHashWithISIS.ini
                 src/hed/profiles/CheloniaAllServicesReplicatedAHashWithISIS/CheloniaAllServicesReplicatedAHashWithISIS.xml
                 src/hed/profiles/CheloniaReplicatedAHash/CheloniaReplicatedAHash.ini
                 src/hed/profiles/CheloniaReplicatedAHash/CheloniaReplicatedAHash.xml
                 src/hed/profiles/CheloniaShepherdWithHopi/CheloniaShepherdWithHopi.ini
                 src/hed/profiles/CheloniaShepherdWithHopi/CheloniaShepherdWithHopi.xml
                 src/hed/profiles/EchoService/EchoService.ini
                 src/hed/profiles/EchoService/EchoService.xml
                 src/hed/profiles/EchoServiceNonSecure/EchoServiceNonSecure.ini
                 src/hed/profiles/EchoServiceNonSecure/EchoServiceNonSecure.xml
                 src/hed/profiles/EchoServicePython/EchoServicePython.ini
                 src/hed/profiles/EchoServicePython/EchoServicePython.xml
                 src/hed/profiles/EchoServicePythonNonSecure/EchoServicePythonNonSecure.ini
                 src/hed/profiles/EchoServicePythonNonSecure/EchoServicePythonNonSecure.xml
                 src/hed/profiles/EchoServiceSAMLToken/EchoServiceSAMLToken.ini
                 src/hed/profiles/EchoServiceSAMLToken/EchoServiceSAMLToken.xml
                 src/hed/profiles/EchoServiceUsernameToken/EchoServiceUsernameToken.ini
                 src/hed/profiles/EchoServiceUsernameToken/EchoServiceUsernameToken.xml
                 src/hed/profiles/EchoServiceX509Token/EchoServiceX509Token.ini
                 src/hed/profiles/EchoServiceX509Token/EchoServiceX509Token.xml
                 src/hed/profiles/HopiService/HopiService.ini
                 src/hed/profiles/HopiService/HopiService.xml
                 src/hed/profiles/HopiServiceNonSecure/HopiServiceNonSecure.ini
                 src/hed/profiles/HopiServiceNonSecure/HopiServiceNonSecure.xml
                 src/hed/profiles/HopiServiceWithPlexer/HopiServiceWithPlexer.ini
                 src/hed/profiles/HopiServiceWithPlexer/HopiServiceWithPlexer.xml
                 src/hed/profiles/HopiServiceWithPlexerNonSecure/HopiServiceWithPlexerNonSecure.ini
                 src/hed/profiles/HopiServiceWithPlexerNonSecure/HopiServiceWithPlexerNonSecure.xml
                 src/hed/profiles/NonSecureNonDistributedStorageElement/NonSecureNonDistributedStorageElement.xml
                 src/hed/profiles/NonSecureP2PIIS/NonSecureP2PIIS.xml
                 src/hed/profiles/NonSecureStandaloneIIS/NonSecureStandaloneIIS.xml
                 src/hed/profiles/SecureComputingElementWithMapfile/SecureComputingElementWithMapfile.ini
                 src/hed/profiles/SecureP2PIIS/SecureP2PIIS.xml
                 src/hed/profiles/SecureStandaloneIIS/SecureStandaloneIIS.xml
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
SAML:               ${enables_saml}
MYSQL CLIENT LIB:   ${enables_mysql}
gSOAP:              ${enables_gsoap}
LDAP:               ${enables_ldap}
xmlsec1:            ${enables_xmlsec1}

Included components:
HED:                ${enables_hed}

A-REX service:      ${enables_a_rex_service}
ISIS service:       ${enables_isis_service}
CHARON service:     ${enables_charon_service}
HOPI service:       ${enables_hopi_service}
SCHED service:      ${enables_sched_service}
STORAGE service:    ${enables_storage_service}
JANITOR service:    ${enables_janitor_service}
PAUL service:       ${enables_paul_service}
COMPILER service:   ${enables_compiler_service}

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

SRM client (DMC):   ${enables_srm_dmc}
GSI channel (MCC):  ${enables_gsi_mcc}

Documentation:      ${enables_doc}
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

