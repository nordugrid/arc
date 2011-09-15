pkglib_LTLIBRARIES = libaccEMIES.la

libaccEMIES_la_SOURCES  = EMIESClient.cpp EMIESClient.h \
	JobStateEMIES.cpp JobStateEMIES.h \
	TargetRetrieverEMIES.cpp TargetRetrieverEMIES.h \
	SubmitterEMIES.cpp SubmitterEMIES.h \
	JobControllerEMIES.cpp JobControllerEMIES.h \
	DescriptorsEMIES.cpp
libaccEMIES_la_CXXFLAGS = -I$(top_srcdir)/include $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libaccEMIES_la_LIBADD = \
	$(top_builddir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la\
	$(top_builddir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libaccEMIES_la_LDFLAGS = -no-undefined -avoid-version -module

#	$(top_builddir)/src/hed/libs/security/libarcsecurity.la \
#	$(top_builddir)/src/hed/libs/data/libarcdata2.la \
#	$(top_builddir)/src/hed/libs/ws/libarcws.la \
#	$(top_builddir)/src/hed/libs/loader/libarcloader.la \
#

#DIST_SUBDIRS = test
#SUBDIRS = $(TEST_DIR)
