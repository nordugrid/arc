lib_LTLIBRARIES = libarcglobusutils.la

libarcglobusutils_la_SOURCES = \
	GlobusErrorUtils.cpp GlobusErrorUtils.h \
	GSSCredential.cpp GSSCredential.h \
	GlobusWorkarounds.cpp GlobusWorkarounds.h

libarcglobusutils_la_CXXFLAGS = \
	-I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) \
	$(GLOBUS_COMMON_CFLAGS) \
	$(OPENSSL_FLAGS)

libarcglobusutils_la_LIBADD = \
	$(ARCCOMMON_LIBS) \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS) \
	$(GLOBUS_COMMON_LIBS) $(GLOBUS_GSSAPI_GSI_LIBS)

libarcglobusutils_la_LDFLAGS  = -version-info 1:0:0
