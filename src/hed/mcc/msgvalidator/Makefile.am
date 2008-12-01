pkglib_LTLIBRARIES = libmccmsgvalidator.la

libmccmsgvalidator_la_SOURCES  = MCCMsgValidator.cpp MCCMsgValidator.h
libmccmsgvalidator_la_CXXFLAGS = $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
libmccmsgvalidator_la_LIBADD   = $(top_srcdir)/src/hed/libs/common/libarccommon.la $(top_srcdir)/src/hed/libs/loader/libarcloader.la $(top_srcdir)/src/hed/libs/message/libarcmessage.la $(top_srcdir)/src/hed/libs/ws/libarcws.la $(LIBXML2_LIBS) $(GLIBMM_LIBS)
libmccmsgvalidator_la_LDFLAGS  = -no-undefined -avoid-version -module
