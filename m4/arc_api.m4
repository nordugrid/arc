
#
# ARC Public API
#

AC_DEFUN([ARC_API], [
ARCCLIENT_LIBS='$(top_builddir)/src/hed/libs/compute/libarccompute.la'
ARCCLIENT_CFLAGS='-I$(top_srcdir)/include'
AC_SUBST(ARCCLIENT_LIBS)
AC_SUBST(ARCCLIENT_CFLAGS)

ARCCOMMON_LIBS='$(top_builddir)/src/hed/libs/common/libarccommon.la'
ARCCOMMON_CFLAGS='-I$(top_srcdir)/include'
AC_SUBST(ARCCOMMON_LIBS)
AC_SUBST(ARCCOMMON_CFLAGS)

ARCCREDENTIAL_LIBS='$(top_builddir)/src/hed/libs/credential/libarccredential.la'
ARCCREDENTIAL_CFLAGS='-I$(top_srcdir)/include'
AC_SUBST(ARCCREDENTIAL_LIBS)
AC_SUBST(ARCCREDENTIAL_CFLAGS)

ARCDATA_LIBS='$(top_builddir)/src/hed/libs/data/libarcdata.la'
ARCDATA_CFLAGS='-I$(top_srcdir)/include'
AC_SUBST(ARCDATA_LIBS)
AC_SUBST(ARCDATA_CFLAGS)

ARCJOB_LIBS='$(top_builddir)/src/hed/libs/job/libarcjob.la'
ARCJOB_CFLAGS='-I$(top_srcdir)/include'
AC_SUBST(ARCJOB_LIBS)
AC_SUBST(ARCJOB_CFLAGS)

ARCLOADER_LIBS='$(top_builddir)/src/hed/libs/loader/libarcloader.la'
ARCLOADER_CFLAGS='-I$(top_srcdir)/include'
AC_SUBST(ARCLOADER_LIBS)
AC_SUBST(ARCLOADER_CFLAGS)

ARCMESSAGE_LIBS='$(top_builddir)/src/hed/libs/message/libarcmessage.la'
ARCMESSAGE_CFLAGS='-I$(top_srcdir)/include'
AC_SUBST(ARCMESSAGE_LIBS)
AC_SUBST(ARCMESSAGE_CFLAGS)

ARCSECURITY_LIBS='$(top_builddir)/src/hed/libs/security/libarcsecurity.la'
ARCSECURITY_CFLAGS='-I$(top_srcdir)/include'
AC_SUBST(ARCSECURITY_LIBS)
AC_SUBST(ARCSECURITY_CFLAGS)

ARCINFOSYS_LIBS='$(top_builddir)/src/hed/libs/infosys/libarcinfosys.la'
ARCINFOSYS_CFLAGS='-I$(top_srcdir)/include'
AC_SUBST(ARCINFOSYS_LIBS)
AC_SUBST(ARCINFOSYS_CFLAGS)

ARCWSADDRESSING_LIBS='$(top_builddir)/src/hed/libs/ws-addressing/libarcwsaddressing.la'
ARCWSADDRESSING_CFLAGS='-I$(top_srcdir)/include'
AC_SUBST(ARCWSADDRESSING_LIBS)
AC_SUBST(ARCWSADDRESSING_CFLAGS)

ARCWSSECURITY_LIBS='$(top_builddir)/src/hed/libs/ws-security/libarcwssecurity.la'
ARCWSSECURITY_CFLAGS='-I$(top_srcdir)/include'
AC_SUBST(ARCWSSECURITY_LIBS)
AC_SUBST(ARCWSSECURITY_CFLAGS)

ARCXMLSEC_LIBS='$(top_builddir)/src/hed/libs/xmlsec/libarcxmlsec.la'
ARCXMLSEC_CFLAGS='-I$(top_srcdir)/include'
AC_SUBST(ARCXMLSEC_LIBS)
AC_SUBST(ARCXMLSEC_CFLAGS)

])
