noinst_PROGRAMS = logger

logger_SOURCES = JobRecord.cpp JobRecord.h client.cpp client.h logger.cpp
logger_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
logger_LDADD = \
	../grid-manager/conf/libconf.la \
	../grid-manager/misc/libmisc.la \
	$(top_builddir)/src/hed/libs/client/libarcclient.la \
	$(top_builddir)/src/hed/libs/message/libarcmessage.la \
	$(top_builddir)/src/hed/libs/common/libarccommon.la
