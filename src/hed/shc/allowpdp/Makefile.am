noinst_LTLIBRARIES = liballowpdp.la

liballowpdp_la_SOURCES = AllowPDP.cpp AllowPDP.h
liballowpdp_la_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
liballowpdp_la_LIBADD = \
	$(top_srcdir)/src/hed/libs/message/libarcmessage.la \
	$(top_srcdir)/src/hed/libs/common/libarccommon.la
