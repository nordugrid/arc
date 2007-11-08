SUBDIRS = ArcPDP

lib_LTLIBRARIES = libarcsecurity.la
libarcsecurity_ladir = $(pkgincludedir)/security
libarcsecurity_la_HEADERS  = SecHandler.h  PDP.h Security.h
libarcsecurity_la_SOURCES  = SecHandler.cpp PDP.cpp Security.cpp
libarcsecurity_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libarcsecurity_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la \
                             $(top_srcdir)/src/hed/libs/loader/libarcloader.la ArcPDP/libarcpdp.la \
                             $(top_srcdir)/src/hed/libs/delegation/libdelegation.la
libarcsecurity_la_LDFLAGS  = $(LIBXML2_LIBS) $(GLIBMM_LIBS) $(GTHREAD_LIBS) $(EXTRA_LDFLAGS)
