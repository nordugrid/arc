pkglib_LTLIBRARIES = libaccARC0.la

libaccARC0_la_SOURCES = \
	TargetRetrieverARC0.cpp TargetRetrieverARC0.h \
	SubmitterARC0.cpp SubmitterARC0.h \
	DescriptorsARC0.cpp \
	JobControllerARC0.cpp JobControllerARC0.h \
	FTPControl.cpp FTPControl.h JobStateARC0.cpp JobStateARC0.h
libaccARC0_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) $(GLOBUS_FTP_CONTROL_CFLAGS)
libaccARC0_la_LIBADD = \
	$(top_builddir)/src/hed/libs/crypto/libarccrypto.la \
	$(top_builddir)/src/hed/libs/globusutils/libarcglobusutils.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/data/libarcdata2.la \
	$(top_builddir)/src/hed/libs/credential/libarccredential.la \
	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS) $(GLOBUS_FTP_CONTROL_LIBS)
libaccARC0_la_LDFLAGS = -no-undefined -avoid-version -module
