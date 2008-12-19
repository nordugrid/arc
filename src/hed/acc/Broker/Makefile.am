pkglib_LTLIBRARIES = libaccBroker.la

libaccBroker_la_SOURCES  = FastestQueueBroker.cpp FastestQueueBroker.h \
                         RandomBroker.cpp RandomBroker.h \
                         DescriptorsBroker.cpp \
                         FastestCPUBroker.cpp FastestCPUBroker.h \
                         DataBroker.cpp DataBroker.h 
libaccBroker_la_CXXFLAGS = $(LIBXML2_CFLAGS) $(GLIBMM_CFLAGS) \
                         -I$(top_srcdir)/include 
libaccBroker_la_LIBADD   = $(LIBXML2_LIBS) $(GLIBMM_LIBS) \
                         $(top_srcdir)/src/hed/libs/data/libarcdata2.la \
                         $(top_srcdir)/src/hed/libs/common/libarccommon.la \
                         $(top_srcdir)/src/hed/libs/loader/libarcloader.la \
                         $(top_srcdir)/src/hed/libs/client/libarcclient.la
libaccBroker_la_LDFLAGS  = -no-undefined -avoid-version -module
