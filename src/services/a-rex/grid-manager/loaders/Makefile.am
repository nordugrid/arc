pkglibexec_PROGRAMS = downloader uploader

downloader_SOURCES = downloader.cpp janitor.cpp janitor.h
downloader_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
downloader_LDADD = \
	../jobs/libjobs.la ../log/liblog.la ../files/libfiles.la \
	../run/librun.la ../conf/libconf.la ../misc/libmisc.la \
	../mail/libmail.la \
	$(top_builddir)/src/hed/libs/data/libarcdata2.la

uploader_SOURCES = uploader.cpp janitor.cpp janitor.h
uploader_CXXFLAGS = -I$(top_srcdir)/include \
	$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS)
uploader_LDADD = \
	../jobs/libjobs.la ../log/liblog.la ../files/libfiles.la \
	../run/librun.la ../conf/libconf.la ../misc/libmisc.la \
	../mail/libmail.la \
	$(top_builddir)/src/hed/libs/data/libarcdata2.la
