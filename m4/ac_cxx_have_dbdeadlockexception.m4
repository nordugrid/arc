dnl @synopsis AC_DBCXX_HAVE_DBDEADLOCKEXXCEPTION
dnl
dnl If the C++ library has a working stringstream, define HAVE_SSTREAM.
dnl
dnl @author Ben Stanley
dnl @version $Id: ac_cxx_have_sstream.m4 3830 2005-06-24 07:01:15Z waananen $
dnl
AC_DEFUN([AC_DBCXX_HAVE_DBDEADLOCKEXCEPTION],
[AC_CACHE_CHECK(whether the Berkeley DB has DbDeadlockException,
ac_cv_dbcxx_dbdeadlockexception,
[
 AC_LANG_SAVE
 AC_LANG_CPLUSPLUS
 AC_TRY_COMPILE([#include <db_cxx.h>],[try { } catch(DbDeadlockException&) { }; return 0;],
 ac_cv_dbcxx_have_dbdeadlockexception=yes, ac_cv_dbcxx_have_dbdeadlockexception=no)
 AC_LANG_RESTORE
])
if test "$ac_cv_dbcxx_have_dbdeadlockexception" = yes; then
  AC_DEFINE(HAVE_DBDEADLOCKEXCEPTION,,[define if the Berkeley DB has DbDeadLockException])
fi
])
