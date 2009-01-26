pkglib_LTLIBRARIES = libaccUNICORE.la

libaccUNICORE_la_SOURCES  = TargetRetrieverUNICORE.cpp TargetRetrieverUNICORE.h \
			 SubmitterUNICORE.cpp SubmitterUNICORE.h \
			 JobControllerUNICORE.cpp JobControllerUNICORE.h \
			 UNICOREClient.cpp UNICOREClient.h \
			 DescriptorsUNICORE.cpp
libaccUNICORE_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) \
			 -I$(top_srcdir)/include
libaccUNICORE_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) \
			 $(top_srcdir)/src/hed/libs/data/libarcdata2.la \
			 $(top_srcdir)/src/hed/libs/common/libarccommon.la \
			 $(top_srcdir)/src/hed/libs/message/libarcmessage.la\
			 $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
			 $(top_srcdir)/src/hed/libs/client/libarcclient.la \
			 $(top_srcdir)/src/hed/libs/security/libarcsecurity.la \
			 $(top_srcdir)/src/hed/libs/delegation/libarcdelegation.la \
			 $(top_srcdir)/src/hed/libs/ws/libarcws.la
libaccUNICORE_la_LDFLAGS  = -no-undefined -avoid-version -module
