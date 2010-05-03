pkglibexec_PROGRAMS=downloader uploader

downloader_SOURCES=downloader.cpp janitor.cpp janitor.h
downloader_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
downloader_LDADD=$(top_srcdir)/src/hed/libs/data/libarcdata2.la $(top_srcdir)/src/hed/libs/client/libarcclient.la ../jobs/libjobs.la ../log/liblog.la ../files/libfiles.la ../run/librun.la ../conf/libconf.la ../misc/libmisc.la ../mail/libmail.la

uploader_SOURCES=uploader.cpp janitor.cpp janitor.h
uploader_CXXFLAGS=$(GLIBMM_CFLAGS) $(LIBXML2_CFLAGS) -I$(top_srcdir)/include
uploader_LDADD=$(top_srcdir)/src/hed/libs/data/libarcdata2.la ../jobs/libjobs.la ../log/liblog.la ../files/libfiles.la ../run/librun.la ../conf/libconf.la ../misc/libmisc.la ../mail/libmail.la

