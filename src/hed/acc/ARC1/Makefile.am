noinst_PROGRAMS = submittest

pkglib_LTLIBRARIES = libaccARC1.la

libaccARC1_la_SOURCES  = TargetRetrieverARC1.cpp TargetRetrieverARC1.h \
			 SubmitterARC1.cpp SubmitterARC1.h \
			 JobControllerARC1.cpp JobControllerARC1.h \
                         DescriptorsARC1.cpp
libaccARC1_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) \
                         -I$(top_srcdir)/include
libaccARC1_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) \
                         $(top_srcdir)/src/hed/libs/data/libarcdata2.la \
                         $(top_srcdir)/src/hed/libs/common/libarccommon.la \
			 $(top_srcdir)/src/hed/libs/message/libarcmessage.la\
			 $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                         $(top_srcdir)/src/hed/libs/client/libarcclient.la
libaccARC1_la_LDFLAGS  = -no-undefined -avoid-version -module

submittest_SOURCES  = submittest.cpp
submittest_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) \
		      -I$(top_srcdir)/include
submittest_LDADD    = $(top_srcdir)/src/hed/libs/client/libarcclient.la \
		      $(top_srcdir)/src/hed/libs/common/libarccommon.la \
		      $(top_srcdir)/src/hed/libs/loader/libarcloader.la

noinst_LTLIBRARIES = libAREXClient.la

libAREXClient_la_SOURCES = arex_client.cpp arex_client.h
libAREXClient_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) \
			    -I$(top_srcdir)/include
