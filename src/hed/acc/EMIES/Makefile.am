pkglib_LTLIBRARIES = libaccEMIES.la

libaccEMIES_la_SOURCES  = EMIESClient.cpp EMIESClient.h \
	JobStateEMIES.cpp JobStateEMIES.h \
	TargetRetrieverEMIES.cpp TargetRetrieverEMIES.h \
	DescriptorsEMIES.cpp
libaccEMIES_la_CXXFLAGS = -I$(top_srcdir)/include $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS)
libaccEMIES_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/delegation/libarcdelegation.la \
	$(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la\
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(LIBXML2_LIBS) $(GLIBMM_LIBS)
libaccEMIES_la_LDFLAGS = -no-undefined -avoid-version -module

#	$(top_srcdir)/src/hed/libs/security/libarcsecurity.la \
#	$(top_srcdir)/src/hed/libs/data/libarcdata2.la \
#	$(top_srcdir)/src/hed/libs/ws/libarcws.la \
#	$(top_srcdir)/src/hed/libs/loader/libarcloader.la \
#

#DIST_SUBDIRS = test
#SUBDIRS = $(TEST_DIR)
