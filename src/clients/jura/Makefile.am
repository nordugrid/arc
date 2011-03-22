pkglibexec_PROGRAMS = jura

jura_SOURCES = main.cpp jura.h UsageReporter.cpp UsageReporter.h \
	JobLogFile.cpp JobLogFile.h Destinations.cpp Destinations.h \
	Destination.cpp Destination.h LutsDestination.cpp LutsDestination.h

jura_CXXFLAGS = -I$(top_srcdir)/include $(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
jura_LDADD = \
        $(top_srcdir)/src/hed/libs/client/libarcclient.la \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la \
	$(GLIBMM_LIBS) $(LIBXML2_LIBS)

man_MANS = jura.1
