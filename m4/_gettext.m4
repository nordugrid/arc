# Fake gettext macros

ifdef([AM_GNU_GETTEXT],[],[
AC_DEFUN([AM_GNU_GETTEXT],[
  AC_MSG_RESULT([There was no gettext support. Maybe You want to add path to gettext.m4 and rerun autoconf again.])
  gettext_available="no"
])
])

ifdef([AM_GNU_GETTEXT_VERSION],[],[
AC_DEFUN([AM_GNU_GETTEXT_VERSION],[])
])

