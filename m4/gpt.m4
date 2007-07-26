# globus.m4                                                    -*- Autoconf -*-
#   Macros to for compiling and linking against globus/gpt packages

AC_DEFUN([GPT_PROG_GPT_FLAVOR_CONFIGURATION], [
AC_ARG_VAR([GPT_FLAVOR_CONFIGURATION], [path to gpt-flavor-configuration])
if test "x$ac_cv_env_GPT_FLAVOR_CONFIGURATION_set" != "xset"; then
   AC_PATH_TOOL([GPT_FLAVOR_CONFIGURATION], [gpt-flavor-configuration], [], $PATH:/opt/gpt/sbin)
fi
if test -f "$GPT_FLAVOR_CONFIGURATION" && test "x$GPT_LOCATION" = "x"; then
   GPT_LOCATION=`dirname $GPT_FLAVOR_CONFIGURATION`
   GPT_LOCATION=`dirname $GPT_LOCATION`
   export GPT_LOCATION
fi
])

AC_DEFUN([GPT_PROG_GPT_QUERY], [
AC_ARG_VAR([GPT_QUERY], [path to gpt-query])
if test "x$ac_cv_env_GPT_QUERY_set" != "xset"; then
   AC_PATH_TOOL([GPT_QUERY], [gpt-query], [], $PATH:/opt/gpt/sbin)
fi
if test -f "$GPT_QUERY" && test "x$GPT_LOCATION" = "x"; then
   GPT_LOCATION=`dirname $GPT_QUERY`
   GPT_LOCATION=`dirname $GPT_LOCATION`
   export GPT_LOCATION
fi
])

AC_DEFUN([GPT_PROG_GLOBUS_MAKEFILE_HEADER], [
AC_ARG_VAR([GLOBUS_MAKEFILE_HEADER], [path to globus-makefile-header])
if test "x$ac_cv_env_GLOBUS_MAKEFILE_HEADER_set" != "xset"; then
   AC_PATH_TOOL([GLOBUS_MAKEFILE_HEADER], [globus-makefile-header], [], $PATH:/opt/globus/bin)
fi
if test -f "$GLOBUS_MAKEFILE_HEADER" && test "x$GLOBUS_LOCATION" = "x"; then
   GLOBUS_LOCATION=`dirname $GLOBUS_MAKEFILE_HEADER`
   GLOBUS_LOCATION=`dirname $GLOBUS_LOCATION`
   export GLOBUS_LOCATION
fi
])

AC_DEFUN([GPT_ARG_GPT_FLAVOR], [
AC_REQUIRE([GPT_PROG_GPT_FLAVOR_CONFIGURATION])
AC_MSG_CHECKING([for gpt flavor])
AC_ARG_WITH([flavor], AC_HELP_STRING([--with-flavor=(flavor)],
		      [Specify the gpt build flavor [[autodetect]]]),
[GPT_FLAVOR=$withval],
if test -n "$GPT_FLAVOR_CONFIGURATION" ; then
   [GPT_FLAVOR=`$GPT_FLAVOR_CONFIGURATION | \\
	grep '^[[a-zA-Z]].*:$' | cut -f1 -d: | grep thr | tail -1`]
fi)
if test -n "$GPT_FLAVOR"; then
   AC_MSG_RESULT($GPT_FLAVOR)
else
   AC_MSG_RESULT(none detected, is globus_core-devel installed?)
fi
])

AC_DEFUN([GPT_PKG_VERSION], [
AC_REQUIRE([GPT_PROG_GPT_QUERY])
AC_REQUIRE([GPT_ARG_GPT_FLAVOR])
if test -n "$GPT_QUERY" && test -n "$GPT_FLAVOR"; then
   gpt_cv_[]$1[]_version=`$GPT_QUERY $1[]-[]$GPT_FLAVOR[]-dev | \\
	grep 'pkg version' | sed 's%.*:\s*%%'`
fi
])

AC_DEFUN([GPT_PKG], [
AC_REQUIRE([GPT_PROG_GLOBUS_MAKEFILE_HEADER])
AC_REQUIRE([GPT_ARG_GPT_FLAVOR])

AC_MSG_CHECKING([for $1])

GPT_PKG_VERSION($1)

if test -n "$gpt_cv_[]$1[]_version"; then
   if test -n "$GLOBUS_MAKEFILE_HEADER" && test -n "$GPT_FLAVOR" ; then
      gpt_cv_tmp=`$GLOBUS_MAKEFILE_HEADER --flavor=$GPT_FLAVOR $1 | \\
		  sed 's%\s*=\s*\(.*\)%="\1"%'`
      gpt_cv_[]$1[]_cflags=`eval "$gpt_cv_tmp" \\
			    echo '$GLOBUS_INCLUDES'`
      gpt_cv_[]$1[]_libs=`eval "$gpt_cv_tmp" \\
			  echo '$GLOBUS_LDFLAGS $GLOBUS_PKG_LIBS $GLOBUS_LIBS'`
      gpt_cv_tmp=
   fi
fi

if test -n "$gpt_cv_[]$1[]_version"; then
   AC_MSG_RESULT($gpt_cv_[]$1[]_version)
   m4_toupper([$1])[]_VERSION=$gpt_cv_[]$1[]_version
   m4_toupper([$1])[]_LIBS=$gpt_cv_[]$1[]_libs
   m4_toupper([$1])[]_CFLAGS=$gpt_cv_[]$1[]_cflags
else
   AC_MSG_RESULT(no)
fi
])
