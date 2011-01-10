pkglib_LTLIBRARIES = libaccARC1.la
noinst_LTLIBRARIES = libarexclient.la
bin_PROGRAMS = arcdeleg

libaccARC1_la_SOURCES  = TargetRetrieverARC1.cpp TargetRetrieverARC1.h \
			 SubmitterARC1.cpp SubmitterARC1.h \
			 JobControllerARC1.cpp JobControllerARC1.h \
			 AREXClient.cpp AREXClient.h \
			 DescriptorsARC1.cpp JobStateARC1.h JobStateARC1.cpp \
			 TargetRetrieverBES.cpp TargetRetrieverBES.h \
			 SubmitterBES.cpp SubmitterBES.h \
			 JobControllerBES.cpp JobControllerBES.h \
			 JobStateBES.cpp JobStateBES.h
libaccARC1_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) \
			 -I$(top_srcdir)/include
libaccARC1_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) \
			 $(top_srcdir)/src/hed/libs/data/libarcdata2.la \
			 $(top_srcdir)/src/hed/libs/common/libarccommon.la \
			 $(top_srcdir)/src/hed/libs/message/libarcmessage.la\
			 $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
			 $(top_srcdir)/src/hed/libs/client/libarcclient.la \
			 $(top_srcdir)/src/hed/libs/security/libarcsecurity.la \
			 $(top_srcdir)/src/hed/libs/delegation/libarcdelegation.la \
			 $(top_srcdir)/src/hed/libs/ws/libarcws.la
libaccARC1_la_LDFLAGS  = -no-undefined -avoid-version -module

libarexclient_la_SOURCES  = AREXClient.cpp AREXClient.h
libarexclient_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) \
			 -I$(top_srcdir)/include
libarexclient_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) \
			 $(top_srcdir)/src/hed/libs/common/libarccommon.la \
			 $(top_srcdir)/src/hed/libs/message/libarcmessage.la\
			 $(top_srcdir)/src/hed/libs/client/libarcclient.la \
			 $(top_srcdir)/src/hed/libs/security/libarcsecurity.la \
			 $(top_srcdir)/src/hed/libs/delegation/libarcdelegation.la \
			 $(top_srcdir)/src/hed/libs/ws/libarcws.la
libarexclient_la_LDFLAGS  =

arcdeleg_SOURCES  = arcdeleg.cpp JobStateBES.cpp JobStateBES.h JobStateARC1.h JobStateARC1.cpp
arcdeleg_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) -I$(top_srcdir)/include
arcdeleg_LDADD    = libarexclient.la $(top_srcdir)/src/hed/libs/common/libarccommon.la
arcdeleg_LDFLAGS  = $(GLIBMM_LIBS) $(LIBXML2_LIBS) $(EXTRA_LIBS)


DIST_SUBDIRS = test
SUBDIRS = $(TEST_DIR)
